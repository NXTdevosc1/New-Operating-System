
// Using similar heap manager algorithm

#include <nos/nos.h>
#include <nos/mm/mm.h>
#include <hmapi.h>
HMIMAGE *_NosKernelHeap; // from physinit.c

// Kl : kernel library
PVOID KRNLAPI KlAllocatePool(
    UINT64 Size,
    UINT Flags)
{
    if (!Size)
        return NULL;

    return oHmbAllocate(_NosKernelHeap, AlignForward(Size, 0x10) >> 4);
}

BOOLEAN KRNLAPI KlFreePool(
    void *Address)
{
    return oHmbFree(_NosKernelHeap, Address);
}