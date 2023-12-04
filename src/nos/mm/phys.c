#include <nos/nos.h>
#include <mm.h>
#include <nos/mm/physmem.h>

PVOID KRNLAPI MmRequestContiguousPagesNoDesc(
    IN HMIMAGE *Image,
    IN UINT64 Length)
{
    return NULL;
}

static inline PVOID MmiIncludeAdditionalPages(
    HMIMAGE *Image,
    UINT64 Length)
{
    return NULL;
}

PVOID KRNLAPI MmRequestContiguousPages(
    UINT PageSize,
    UINT64 Length)
{
    return NULL;
}

void KRNLAPI MmFreePages(
    PVOID Address)
{
}
