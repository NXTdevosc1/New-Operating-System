#include <nos/nos.h>
#include <nos/mm/mm.h>

#define PAGE_HALFPTR ((UINT64)0x8000000)


PVOID MmiPreosAllocateMemory(
    IN UINT64 NumPages,
    IN UINT64 PageAttributes,
    IN UINT64 CachePolicy
 ) {
    UINT64 Flags = 0;
    if(PageAttributes & PAGE_2MB) Flags |= ALLOCATE_2MB_ALIGNED_MEMORY;
    void* Ptr;
    if(NERROR(MmAllocatePhysicalMemory(Flags, NumPages, &Ptr))) {
        return NULL;
    }
    void* VirtualAddress = HwFindAvailableAddressSpace(
        GetCurrentPageTable(),
        NumPages,
        NosInitData->NosKernelImageBase,
        (void*)-1,
        PageAttributes
    );
    if(!VirtualAddress) {
        // FREE MEMORY
        KDebugPrint("MM_PREOS_ALLOC_MEM FAILED");
        while(1);
    }
    UINT64 LogicalPages = (PageAttributes & PAGE_2MB) ? (NumPages << 9) : (NumPages);
    // Map the pages then release the process lock
    HwMapVirtualMemory(
        GetCurrentPageTable(),
        Ptr,
        VirtualAddress,
        NumPages,
        PageAttributes,
        CachePolicy
    );
    return VirtualAddress;
}

PVOID KRNLAPI MmAllocateMemory(
    IN PEPROCESS Process,
    IN UINT64 NumPages,
    IN UINT64 PageAttributes,
    IN UINT64 CachePolicy    
) {
    if(!KernelProcess) return MmiPreosAllocateMemory(NumPages, PageAttributes, CachePolicy);
    
    if(!Process) Process = KernelProcess;
    
    // TODO : Allocate Fragmented memory instead of contiguous memory
    UINT64 Flags = 0;
    if(PageAttributes & PAGE_2MB) Flags |= ALLOCATE_2MB_ALIGNED_MEMORY;
    if(PageAttributes & PAGE_HALFPTR) Flags |= ALLOCATE_BELOW_4GB;

    PageAttributes &= ~PAGE_HALFPTR;

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
        CachePolicy
    );
    ProcessReleaseControlLock(Process, PROCESS_CONTROL_MANAGE_ADDRESS_SPACE);
    return VirtualAddress;
}

BOOLEAN KRNLAPI MmFreeMemory(
    IN PEPROCESS Process,
    IN void *Mem,
    IN UINT64 NumPages
) {
    void* PhysMem = KeConvertPointer(Process, Mem);
    if(!PhysMem) return FALSE;
    return MmFreePhysicalMemory(PhysMem, NumPages);
}

NSTATUS KRNLAPI MmShareMemory(
    IN PEPROCESS Source,
    IN PEPROCESS Destination,
    IN OUT void** VirtualAddress,
    IN UINT64 NumPages,
    IN UINT64 PageAttributes,
    IN UINT CachePolicy
) {
    void* Vmem = KeFindAvailableAddressSpace(Destination, NumPages, NULL, NULL, PageAttributes);
    if(!Vmem) {
        return STATUS_OUT_OF_MEMORY;
    }
    void* PhysAddr = KeConvertPointer(Source, *VirtualAddress);
    
    if(NERROR(KeMapVirtualMemory(Destination, PhysAddr, Vmem, NumPages, PageAttributes, CachePolicy))) {
        KDebugPrint("MmShareMem : ERR0");
        while(1) __halt();
    }
    *VirtualAddress = Vmem;
    ProcessReleaseControlLock(Destination, PROCESS_CONTROL_MANAGE_ADDRESS_SPACE);
    return STATUS_SUCCESS;
}