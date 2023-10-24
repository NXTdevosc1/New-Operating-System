
// Using similar heap manager algorithm

#include <nos/nos.h>
#include <nos/mm/mm.h>

/*
 * Used for sized allocations
 */
static UINT64 SizeTree0 = 0;
static UINT64 SizeTree1[64] = {0};

typedef struct
{
    UINT64 Address;
    UINT64 Length;
} HEAPBLK, *PHEAPBLK;

static PHEAPBLK Recent = NULL;

void *KeoAllocateBlock(UINT64 Length)
{
    if (Length > 0x1000)
    {
        // allocate in pages
    }
    if (Recent->Length < Length)
    {
        //
    }
}