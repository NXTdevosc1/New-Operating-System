#include <nos/nos.h>
#include <nos/mm/mm.h>

NOS_HEAP_TREE KernelHeapTree = {0};
SPINLOCK HeapSpinlock = 0;
NOS_HEAP* InitialHeap = NULL;
NOS_HEAP* LastHeap = NULL;
#define HEAP_NOTABLE_FLAGS 0

static inline NOS_HEAP* MmAllocateDescriptor(void* Address, UINT64 Size, UINT Flags) {
    NOS_HEAP_TREE* Tree = &KernelHeapTree;
    UINT Index, IndexTreeChild, IndexHeap;
    UINT64 Mask;
    NOS_HEAP_TREE_CHILD* TreeChild;
    NOS_HEAP_LIST* HeapList;
    for(;;Tree = Tree->Next) {
        if(!_BitScanForward64(&Index, ~Tree->FullSlots)) {
            if(!Tree->Next) {
                if(NERROR(AllocateNextList(Tree->Next))) {
                    SerialLog("MmAllocDesc : ERR0");
                    while(1);
                }
            }
            continue;
        }

        if(!_bittest64(&Tree->ActiveSlots, Index)) {
            if(NERROR(AllocateNextList(Tree->Slots[Index].Child))) {
                SerialLog("MmAllocDesc : ERR1");
                while(1);
            }
            Tree->Slots[Index].Child->ActiveSlots = 0;
            Tree->Slots[Index].Child->FullSlots = 0;
            Tree->Slots[Index].Child->Parent = Tree;
            Tree->Slots[Index].Child->EntryIndex = Index;


            Tree->ActiveSlots |= (1 << Index);
        }
        TreeChild = Tree->Slots[Index].Child;
        if(!_BitScanForward64(&IndexTreeChild, ~TreeChild->FullSlots)) {
            SerialLog("MmAllocDesc : BUG0");
            while(1);
        }
        if(!_bittest64(&TreeChild->ActiveSlots, IndexTreeChild)) {
            if(NERROR(AllocateNextList(TreeChild->Slots[IndexTreeChild].Child))) {
                SerialLog("MmAllocDesc : ERR2");
                while(1);
            }
            TreeChild->Slots[IndexTreeChild].Child->ActiveHeaps = 0;
            TreeChild->Slots[IndexTreeChild].Child->Parent = TreeChild;
            TreeChild->Slots[IndexTreeChild].Child->EntryIndex = IndexTreeChild;

            TreeChild->ActiveSlots |= (1 << Index);

        }
        HeapList = TreeChild->Slots[IndexTreeChild].Child;
        if(!_BitScanForward64(&IndexHeap, ~HeapList->ActiveHeaps)) {
            SerialLog("MmAllocDesc : BUG1");
            while(1);
        }
        HeapList->ActiveHeaps |= (1 << IndexHeap);
        NOS_HEAP* Heap = HeapList->Heaps + IndexHeap;
        Heap->Address = Address;
        Heap->Size = Size;
        Heap->Flags = Flags;

        // Set full heaps
        if(HeapList->ActiveHeaps == (UINT64)-1) TreeChild->FullSlots |= (1 << IndexTreeChild);
        if(TreeChild->FullSlots == (UINT64)-1) Tree->FullSlots |= (1 << Index);
        
        return Heap;
    }
}

static inline void MmRemoveHeap(NOS_HEAP* Heap) {
    
    NOS_HEAP_LIST* List = (NOS_HEAP_LIST*)AlignBackward((UINT64)Heap, 0x1000);
    UINT HeapIndex = ((UINT64)Heap - (UINT64)List->Heaps) / sizeof(NOS_HEAP);
    _bittestandreset64(&List->ActiveHeaps, HeapIndex);
    Heap->Next = NULL;
    _bittestandreset64(&List->Parent->FullSlots, List->EntryIndex);
    _bittestandreset64(&List->Parent->Parent->FullSlots, List->Parent->EntryIndex);
}
static inline NOS_HEAP* MmFindLinkeableHeap(
    void* Address,
    UINT Flags
) {
    if(!InitialHeap) return NULL;
    NOS_HEAP* Heap = InitialHeap;
    for(;Heap;Heap = Heap->Next) {
        UINT64 addr = (UINT64)Heap->Address + Heap->Size;
        if(addr > (UINT64)Address) break;
        if(addr == (UINT64)Address) {
            // pages are not of the same type
            if(Heap->Flags != Flags) return NULL;
            // heap is linkeable
            return Heap;
        }
    }
    return NULL;
}



NOS_HEAP* MmCreateHeap(
    void* Address,
    UINT64 Size,
    UINT Flags
) {
    
    NOS_HEAP* Heap = MmFindLinkeableHeap((void*)((UINT64)Address), Flags);
    if(!Heap) {
        Heap = MmAllocateDescriptor(Address, Size, Flags);
        if(LastHeap) {
            LastHeap->Next = Heap;
            LastHeap = Heap;
        } else {
            InitialHeap = Heap;
            LastHeap = Heap;
        }
    } else {
        // SerialLog("Merging heaps...")
        Heap->Size+=Size;
    }
    return Heap;
}

typedef struct _BLOCK_HEADER {
    UINT64 Size;
    struct _BLOCK_HEADER* Next;
} BLOCK_HEADER;

BLOCK_HEADER* BlockStart = NULL;
BLOCK_HEADER* BlockEnd = NULL;

static inline void MmAddBlock(BLOCK_HEADER* Bh, UINT64 Size) {
    Bh->Size = Size - sizeof(BLOCK_HEADER);
    Bh->Next = NULL;
    if(!BlockEnd) {
        BlockStart = Bh;
        BlockEnd = Bh;
    } else {
        BlockEnd->Next = Bh;
        BlockEnd = Bh;
    }
}

PVOID KRNLAPI MmAllocatePool(
    UINT64 Size,
    UINT Flags
)
{
    Size = AlignForward(Size, HEAP_ALIGNMENT);
    Size += sizeof(BLOCK_HEADER);
    NOS_HEAP* Heap = InitialHeap;
    NOS_HEAP* PrevHeap = NULL;
    UINT64 CpuFlags;
    void* Address;
    CpuFlags = KeAcquireSpinLock(&HeapSpinlock);
    Flags &= HEAP_NOTABLE_FLAGS;
    for(;Heap;Heap = Heap->Next, PrevHeap = Heap) {
        if(Heap->Flags == Flags && Heap->Size >= Size) {
            Heap->Size -= Size;
            Address = Heap->Address;
            MmAddBlock(Address, Size);
            (UINT64)Heap->Address += Size;
            if(!Heap->Size) {
                // Remove heap
                if(PrevHeap) {
                    PrevHeap->Next = Heap->Next;
                } else {
                    InitialHeap = NULL;
                }
                MmRemoveHeap(Heap);
            }
            KeReleaseSpinLock(&HeapSpinlock, CpuFlags);
            return (void*)((UINT64)Address + sizeof(BLOCK_HEADER));
        }
    }
    // Allocate new heap
    /*
CPU TLB Optimization protocol: Allocate Large pages if possible then small pages
    */
    UINT64 PageAttributes = PAGE_WRITE_ACCESS;

    UINT64 Num4KBPages = ConvertToPages(Size);
    UINT64 NumLargePages = Num4KBPages >> 9;
    
//    SerialLog("AllocatePool : Requesting Pages...");
    Address = KeFindAvailableAddressSpace(
        KernelProcess, Num4KBPages, KernelProcess->VmSearchStart, KernelProcess->VmSearchEnd, 0
    );
    if(!Address) {
        SerialLog("AllocatePool : ERR1")
        // FREE MEMORY
        while(1);
    }
    KernelProcess->VmSearchStart = (void*)((UINT64)Address + (Num4KBPages << 12));
    
    void* PhysicalAddress = NULL; // 2MB PAGES
    void* PhysicalAddress2 = NULL; // 4KB PAGES
    if(NumLargePages) {
        if(NSUCCESS(MmAllocatePhysicalMemory(ALLOCATE_2MB_ALIGNED_MEMORY, NumLargePages, &PhysicalAddress))) {
            Num4KBPages -= (NumLargePages << 9);
        }
    }

    if(Num4KBPages) {
        if(NERROR(MmAllocatePhysicalMemory(0, Num4KBPages, &PhysicalAddress2))) {
            SerialLog("malloc ERR0");
            while(1);
        }
    }

    if(PhysicalAddress) {
        KeMapVirtualMemory(
            KernelProcess, PhysicalAddress, Address, NumLargePages, PageAttributes | PAGE_2MB, 0
        );
        if(PhysicalAddress2) {
            KeMapVirtualMemory(
            KernelProcess, PhysicalAddress2, (void*)((UINT64)Address + (NumLargePages << 21)), Num4KBPages, PageAttributes, 0
            );
        }
    } else {
        KeMapVirtualMemory(
            KernelProcess, PhysicalAddress2, Address, Num4KBPages, PageAttributes, 0
        );
    }
    ProcessReleaseControlLock(KernelProcess, PROCESS_CONTROL_ALLOCATE_ADDRESS_SPACE);

    if(Size != ((Num4KBPages << 12) + (NumLargePages << 21))) {
        // Now create a heap descriptor
        Heap = MmCreateHeap(
            (void*)(Address),
            (Num4KBPages << 12) + (NumLargePages << 21),
            Flags
        );
        (UINT64)Heap->Address += Size;
        (UINT64)Heap->Size -= Size;
    }

    MmAddBlock(Address, Size);

    KeReleaseSpinLock(&HeapSpinlock, CpuFlags);

    return (void*)((UINT64)Address + sizeof(BLOCK_HEADER));
}