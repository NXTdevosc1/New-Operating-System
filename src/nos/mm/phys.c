#include <nos/nos.h>
#include <mm.h>
#include <nos/mm/physmem.h>

static BOOLEAN Bgchk = FALSE;

static void __forceinline __fastcall __MmFillRemainingPages(KPAGEHEADER *Desc, UINT Level, UINT64 Length)
{
    if (!Length)
        return;
    const UINT64 Count = Length & 0x1FF;

#ifdef DEBUG
    if (!Count)
    {
        KDebugPrint("PH BUG0");
        while (1)
            __halt();
    }
#endif

    VmmInsert(VmmPageLevel(_NosPhysicalMemoryImage, Level), Desc, Count);
    __MmFillRemainingPages(Desc + (Count << (9 * Level)), Level + 1, Length >> 9);
}

PVOID KRNLAPI MmRequestContiguousPages(
    UINT PageSize,
    UINT64 Length)
{

    UINT64 Count = Length;
    PVOID ret = NULL;
    KPAGEHEADER *Header;
    for (UINT i = PageSize; i < 3; i++, Count = AlignForward(Count, 0x200) >> 9)
    {

        if ((ret = VmmAllocate(_NosPhysicalMemoryImage, i, Count, &Header)))
        {

            UINT64 Remaining = (Count << (9 * (i - PageSize))) - (Length);

            if (Remaining)
                __MmFillRemainingPages(Header + (Length << (9 * PageSize)), PageSize, Remaining);

            return ret;
        }
    }
    return NULL;
}

void KRNLAPI MmFreePages(
    PVOID Address)
{
}
