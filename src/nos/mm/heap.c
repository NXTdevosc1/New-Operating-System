#include <nos/nos.h>
#include <nos/mm/mm.h>
#include <nos/mm/vm.h>

PVOID KRNLAPI MmAllocatePool(
    IN PROCESS* Process,
    IN UINT64 NumBytes,
    IN UINT64 PoolFlags // Currently Unused, will specify paging flags (disk pages, compressions...)
) {
    PoolFlags &= PAGE_FLAGS_UNTOLERABLE_BITMASK;
    NOS_VIRTUAL_MEMORY_LIST* Vm = &Process->VirtualMemory;
    PVOID Address;
    while(Vm) {
        UINT Index;
        UINT64 Mask = Vm->Present;
        while(_BitScanForward64(&Index, Mask)) {
            _bittestandreset64(&Mask, Index);
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

NOS_HEAP* MmAllocateHeap(NOS_HEAP_TREE* HeapTree) {
    NOS_HEAP_TREE_CHILD* Child;
    NOS_HEAP_LIST* HeapList;
    for(;;) {
        UINT Index;
        UINT64 CpuFlags = KeAcquireSpinLock(&HeapTree->ControlLock);
        UINT64 Mask = ~HeapTree->ActiveSlots;
        if(_BitScanForward64())
        
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