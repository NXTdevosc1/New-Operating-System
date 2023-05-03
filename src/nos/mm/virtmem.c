#include <nos/nos.h>
#include <nos/mm/mm.h>
#include <nos/mm/vm.h>

PVOID KRNLAPI MmAllocateMemory(
    IN PROCESS* Process,
    IN UINT64 NumPages,
    IN UINT64 PageAttributes    
) {
    // TODO : Allocate Fragmented memory instead of contiguous memory
    UINT64 Flags = 0;
    if(PageAttributes & PAGE_2MB) Flags |= ALLOCATE_2MB_ALIGNED_MEMORY;
    void* Ptr;
    if(NERROR(MmAllocatePhysicalMemory(Flags, NumPages, &Ptr))) {
        return NULL;
    }
    void* VirtualAddress;
    if(ProcessOperatingMode(Process) == KERNEL_MODE) {
    if(!Process->VmSearchStart) Process->VmSearchStart = (void*)0xFFFF800000000000;
        VirtualAddress = KeFindAvailableAddressSpace(
            Process,
            NumPages,
            Process->VmSearchStart,
            (void*)(UINT64)-1,
            PageAttributes
        );
    } else {
        if(!Process->VmSearchStart) Process->VmSearchStart = (void*)0x1000;
        VirtualAddress = KeFindAvailableAddressSpace(
            Process,
            NumPages,
            Process->VmSearchStart,
            (void*)0x800000000000,
            PageAttributes
        );
    }
    if(!VirtualAddress) {
        // FREE MEMORY
        while(1);
    }
    UINT64 LogicalPages = (PageAttributes & PAGE_2MB) ? (NumPages << 9) : (NumPages);
    Process->VmSearchStart = (void*)((UINT64)VirtualAddress + (LogicalPages << 12));
    // Map the pages then release the process lock
    KeMapVirtualMemory(
        Process,
        Ptr,
        VirtualAddress,
        NumPages,
        PageAttributes,
        0
    );
    ProcessReleaseControlLock(Process, PROCESS_CONTROL_ALLOCATE_ADDRESS_SPACE);
    VmCreateDescriptor(
        Process,
        VirtualAddress,
        NumPages,
        PageAttributes
    );
    return VirtualAddress;
}