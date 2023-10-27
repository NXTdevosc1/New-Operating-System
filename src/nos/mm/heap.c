
// Using similar heap manager algorithm

#include <nos/nos.h>
#include <nos/mm/mm.h>
#include <hmapi.h>
static PHEAPBLK Recent = NULL;
HMIMAGE *_NosKernelHeap; // from physinit.c

PVOID KRNLAPI MmAllocatePool(
    UINT64 Size,
    UINT Flags)
{
    return oHmbAllocate(_NosKernelHeap, Size);
}

BOOLEAN KRNLAPI MmFreePool(
    void *Address);