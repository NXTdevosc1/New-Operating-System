#include <nos/nos.h>
#include <mm.h>
#include <nos/mm/physmem.h>

static void __forceinline __fastcall __MmFillRemainingPages(KPAGEHEADER *Desc, UINT Level, UINT64 Length)
{
    if (!Length)
        return;
    const UINT64 Count = Length & 0x1FF;
    if (!Count)
    {
        KDebugPrint("PH BUG0");
        while (1)
            __halt();
    }
    // KDebugPrint("Fill %x-%x lvl %x count %d", Desc->Header.Address << 12, (Desc->Header.Address + Count) << 12, Level, Count);
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
        // KDebugPrint("lvl %d cnt %d", i, Count);
        if ((ret = VmmAllocate(_NosPhysicalMemoryImage, i, Count, &Header)))
        {
            UINT64 Remaining = (Count << (9 * (i - PageSize))) - (Length);

            // KDebugPrint("Remaining : %d pages", Remaining);

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
