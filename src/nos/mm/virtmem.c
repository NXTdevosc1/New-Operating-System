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
    // VmCreateDescriptor(Process, )
}