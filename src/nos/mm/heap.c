#include <nos/nos.h>
#include <nos/mm/mm.h>

NOS_HEAP_TREE KernelHeapTree = {0};
SPINLOCK HeapSpinlock = 0;
NOS_HEAP* InitialHeap = NULL;
NOS_HEAP* LastHeap = NULL;

#define HEAP_2MB (1)

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
    Flags &= HEAP_NOTABLE_FLAGS;
    NOS_HEAP* Heap = InitialHeap;
    for(;Heap;Heap = Heap->Next) {
        UINT64 addr = (UINT64)Heap->Address + Heap->Size;
        if(addr > (UINT64)Address) break;
        if(addr == (UINT64)Address) {
            // pages are not of the same type
            if((Heap->Flags & HEAP_NOTABLE_FLAGS) != Flags) return NULL;
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

#define BLOCK_HEADER_MAGIC 0xCFDA9049

typedef struct _BLOCK_HEADER {
    UINT32 Magic;
    UINT32 Flags;
    UINT64 Size;
    struct _BLOCK_HEADER* Prev;
    struct _BLOCK_HEADER* Next;
} BLOCK_HEADER;

BLOCK_HEADER* BlockStart = NULL;
BLOCK_HEADER* BlockEnd = NULL;

static inline void MmAddBlock(BLOCK_HEADER* Bh, UINT64 Size, UINT32 Flags) {
    Bh->Magic = BLOCK_HEADER_MAGIC;
    Bh->Size = Size - sizeof(BLOCK_HEADER);
    Bh->Flags = Flags;
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
    CpuFlags = ExAcquireSpinLock(&HeapSpinlock);
    Flags &= HEAP_NOTABLE_FLAGS;
    for(;Heap;Heap = Heap->Next, PrevHeap = Heap) {
        if((Heap->Flags & HEAP_NOTABLE_FLAGS) == Flags && Heap->Size >= Size) {
            Heap->Size -= Size;
            Address = Heap->Address;
            MmAddBlock(Address, Size, Flags);
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
            ExReleaseSpinLock(&HeapSpinlock, CpuFlags);
            return (void*)((UINT64)Address + sizeof(BLOCK_HEADER));
        }
    }
    // Allocate new heap
    /*
CPU TLB Optimization protocol: Allocate Large pages for kernel
    */
   UINT64 InitialSize = Size;
    UINT64 PageAttributes = PAGE_WRITE_ACCESS | PAGE_2MB;
    Size = AlignForward(Size, 0x200000);
    UINT64 NumLargePages = Size >> 21;
    
    Address = MmAllocateMemory(
        KernelProcess,
        NumLargePages,
        PageAttributes,
        0
    );


    // Now create a heap descriptor
    Heap = MmCreateHeap(
        (void*)(Address),
        (NumLargePages << 21),
        Flags
    );
    (UINT64)Heap->Address += InitialSize;
    (UINT64)Heap->Size -= InitialSize;

    MmAddBlock(Address, Size, Flags);

    ExReleaseSpinLock(&HeapSpinlock, CpuFlags);

    return (void*)((UINT64)Address + sizeof(BLOCK_HEADER));
}

static inline MmiDetachBlock(BLOCK_HEADER* bh) {
    
}

// *********UNIMPLEMENTED*********
BOOLEAN MmFreePool(IN void* Address) {
    // later
    return TRUE;
    BLOCK_HEADER* bh = (BLOCK_HEADER*)Address - 1;
    if(bh->Magic != BLOCK_HEADER_MAGIC) return FALSE;
    UINT32 PageAlign = 0x1000;
    if(bh->Flags & PAGE_2MB) PageAlign = 0x200000;
    if(!ExcessBytes(bh, PageAlign)) {
        // Free the page

    
    } else {
        NOS_HEAP* Heap = MmCreateHeap(Address, bh->Size, bh->Flags);
        if(!Heap) {
            SerialLog("BUG0 : MmFreePool");
            while(1);
        }
    }
    return TRUE;
}