#include <nos/nos.h>
#include <mm.h>
#include <nos/mm/physmem.h>

static void __forceinline __fastcall __MmFillRemainingPages(UINT64 Addr, UINT Level, UINT64 Length)
{
    UINT Count = Length & 0x1FF;
    KDebugPrint("Fill %x lvl %x count %x", Addr << 12, Level, Count);
    // VmmInsert()
    Length -= Count;
    if (Length)
        __MmFillRemainingPages(Addr + (Count << (9 * Level)), Level + 1, Length >> 9);
}

PVOID KRNLAPI MmRequestContiguousPages(
    UINT PageSize,
    UINT64 Length)
{

    UINT64 Count = Length;
    PVOID ret = NULL;

    for (UINT i = PageSize; i < 4; i++, Count = AlignForward(Count, 0x200) >> 9)
    {
        KDebugPrint("lvl %d cnt %d", i, Count);
        if ((ret = VmmAllocate(_NosPhysicalMemoryImage, i, Count)))
        {
            UINT64 Remaining = (Count << (9 * i)) - (Length);

            KDebugPrint("Remaining : %d pages", Remaining);

            if (Remaining)
            {
                __MmFillRemainingPages(((UINT64)ret >> 12) + (Length << (9 * PageSize)), PageSize, Remaining);
            }
            return ret;
        }
    }
    return NULL;
}

void KRNLAPI MmFreePages(
    PVOID Address)
{
}
