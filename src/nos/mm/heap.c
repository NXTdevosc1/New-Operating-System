#include <nos/nos.h>
#include <nos/mm/mm.h>
#include <nos/mm/vm.h>

PVOID KRNLAPI KeAllocatePool(
    IN UINT64 NumBytes,
    IN UINT64 PoolFlags // Currently Unused, will specify paging flags (disk pages, compressions...)
) {
    PROCESS* Process = KeQueryProcessorById(0);
    PoolFlags &= PAGE_FLAGS_UNTOLERABLE_BITMASK;
    AlignForward(NumBytes, HEAP_ALIGNMENT);

    NOS_VIRTUAL_MEMORY_LIST* Vm = &Process->VirtualMemory;
    PVOID Address;
    while(Vm) {
        UINT Index;
        UINT64 Mask = Vm->Present;
        while(_BitScanForward64(&Index, Mask)) {
            _bittestandreset64(&Mask, Index);
            NOS_VIRTUAL_MEMORY_DESCRIPTOR* desc = Vm->Vm + Index;
            if(desc->LargestHeapLength >= NumBytes) {
                // Search in the heaps
                
            }
            if((Vm->Vm[Index].NumBytes - Vm->Vm[Index].UsedMemory) >= NumBytes) {

            }
        }
    }

    // No enough memory for the process, allocate new memory
    UINT64 Num4KBPages = AlignForward(NumBytes, 0x1000) >> 12;
    UINT64 NumLargePages = Num4KBPages >> 9;
    if(ProcessOperatingMode(Process) == KERNEL_MODE) {
        if(!Process->VmSearchStart) Process->VmSearchStart = (void*)0xFFFF800000000000;
        Address = KeFindAvailableAddressSpace(
            Process,
            Num4KBPages,
            Process->VmSearchStart,
            (void*)(UINT64)-1,
            PoolFlags
        );
    } else {
        if(!Process->VmSearchStart) Process->VmSearchStart = (void*)0x1000;
        Address = KeFindAvailableAddressSpace(
            Process,
            Num4KBPages,
            Process->VmSearchStart,
            (void*)0x800000000000,
            PoolFlags
        );
    }
    // Now we are holding address space control
    PVOID Tmp = Address;
    if(NumLargePages) {
        PVOID Phys;
        if(NSUCCESS(MmAllocatePhysicalMemory(ALLOCATE_2MB_ALIGNED_MEMORY, NumLargePages, &Phys))) {
            KeMapVirtualMemory(Process, Phys, Tmp, NumLargePages, PoolFlags | PAGE_2MB, 0);
            (UINT64)Tmp += (NumLargePages << 21);
            Num4KBPages -= (NumLargePages << 9);
            
        }
    }
    if(Num4KBPages) {
        PVOID Phys;
        if(NERROR(MmAllocatePhysicalMemory(0, Num4KBPages, &Phys))) {
            SerialLog("AllocPool : Failed to allocate phys mem.");
            while(1);
        }
        KeMapVirtualMemory(Process, Phys, Tmp, Num4KBPages, PoolFlags, 0);
    }
    VmCreateDescriptor(Process, Address, Num4KBPages, PoolFlags);

    return Address;
}

BOOLEAN MmCreateHeap(
    NOS_HEAP_TREE* HeapTree,
    void* Address,
    UINT64 NumBytes,
    UINT64 Flags
) {
    NOS_HEAP_TREE_CHILD* Child;
    NOS_HEAP_LIST* HeapList;
    NOS_HEAP* Heap;
    for(;;) {
        UINT Index;
        UINT64 CpuFlags = KeAcquireSpinLock(&HeapTree->ControlLock);
        if(HeapTree->FullSlots == (UINT64)-1) goto cancel0;
        UINT64 Mask = (HeapTree->ActiveSlots) & (~HeapTree->FullSlots);
        if(_BitScanForward64(&Index, (~HeapTree->FullSlots))) {
            if(!_bittest64(&HeapTree->ActiveSlots, Index)) {
                if(NERROR(AllocateObject(HeapTree->Slots[Index].Child))) {
                    SerialLog("MM_ALLOC_HEAP : AllocateObject0 Failed");
                    while(1);
                }
                HeapTree->Slots[Index].Child->ActiveSlots = 0;
                HeapTree->Slots[Index].Child->FullSlots = 0;
            }
            KeCheckMutexEnter(NULL, &HeapTree->Slots[Index].Mutex);
            // Temporary set as full slot to prevent errors
            _bittestandset64(&HeapTree->FullSlots, Index);
            KeReleaseSpinLock(&HeapTree->ControlLock, CpuFlags);
            Child = HeapTree->Slots[Index].Child;
        } else {
            SerialLog("MM_ALLOC_HEAP : BUG0");
            while(1);
        }
        // Now Search in tree child
        UINT IndexChild, IndexList;
        if(!_BitScanForward64(&IndexChild, ~Child->FullSlots)) {
            // BUG
            SerialLog("MM_ALLOC_HEAP : BUG1");
            while(1);
        }
        if(!_bittest64(&Child->ActiveSlots, IndexChild)) {
            if(NERROR(AllocateObject(Child->Slots[IndexChild].Child))) {
                SerialLog("MM_ALLOC_HEAP : AllocateObject1 Failed");
                while(1);
            }
            Child->Slots[IndexChild].Child->ActiveHeaps = 0;
        }
        HeapList = Child->Slots[IndexChild].Child;
        if(!_BitScanForward64(&IndexList, ~HeapList->ActiveHeaps)) {
            SerialLog("MM_ALLOC_HEAP : BUG2");
            while(1);
        }
        Heap = HeapList->Heaps + IndexList;
        Heap->Address = Address;
        Heap->Flags = Flags;
        Heap->NumBytes = NumBytes;
        _bittestandset64(&HeapList->ActiveHeaps, IndexList);
        // Check full slots
        if(HeapList->ActiveHeaps == (UINT64)-1) _bittestandset64(&Child->FullSlots, IndexChild);
        if(Child->FullSlots == (UINT64)-1) _interlockedbittestandset64(&HeapTree->FullSlots, Index);
        else _interlockedbittestandreset64(&HeapTree->FullSlots, Index);
        KeMutexRelease(NULL, &HeapTree->Slots[Index].Mutex);
        
        return TRUE;
cancel0:
        KeReleaseSpinLock(&HeapTree->ControlLock, CpuFlags);
        if(!HeapTree->Next) {
            if(NERROR(AllocateNextList(HeapTree->Next))) {
                SerialLog("MMAllocHeap : NEXT_LIST_ALLOC_FAIL");
                while(1);
            }
        }
        HeapTree = HeapTree->Next;
    }
}