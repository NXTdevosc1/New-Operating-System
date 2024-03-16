#include <nos/nos.h>
#include <nos/mm/physmem.h>
#include <nos/mm/internal.h>

PVOID __fastcall iRequestPhysicalMemory(
    PEPROCESS Process,
    UINT Flags,
    UINT64 Length)
{
    HMIMAGE *Image = _NosPhysicalMemoryImage;
    if (Flags & ALLOCATE_BELOW_4GB)
    {
        Image = _PhysicalMemoryBelow4GB;
        KDebugPrint("ALLOCATE_MEM < 4GB");
        while (1)
            __halt();
    }

    UINT64 Count = Length;
    PVOID ret = NULL;
    KPAGEHEADER **Header;
    // TODO : Auto page size
    UINT PageSize = 0;
    if (Flags & MEM_LARGE_PAGES)
        PageSize = 1;
    if (Flags & MEM_HUGE_PAGES)
        PageSize = 2;

    for (UINT i = PageSize; i < 3; i++, Count = AlignForward(Count, 0x200) >> 9)
    {

        if ((ret = VmmAllocate(_NosPhysicalMemoryImage, i, Count, &Header)))
        {

            UINT64 Remaining = (Count << (9 * (i - PageSize))) - (Length);

            if (Remaining)
                __MmFillRemainingPages((*Header) + (Length << (9 * PageSize)), PageSize, Remaining);

            *Header += (Length << (9 * PageSize));

            return ret;
        }
    }
    return NULL;
}

void KRNLAPI KFreePhysicalMemory(
    PVOID Address)
{
}
