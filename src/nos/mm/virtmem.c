#include <nos/nos.h>
#include <nos/mm/mm.h>

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
    void* VirtualAddress = KeFindAvailableAddressSpace(
        Process,
        NumPages,
        Process->VmSearchStart,
        Process->VmSearchEnd,
        PageAttributes
    );
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
    return VirtualAddress;
}