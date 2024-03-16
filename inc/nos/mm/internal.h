#pragma once
#include <nosdef.h>
#include <nos/mm/physmem.h>

typedef PVOID(__fastcall *MEMREQUEST)(PEPROCESS Process, UINT Flags, UINT64 Length);

PVOID __fastcall iRequestPhysicalMemory(
    PEPROCESS Process,
    UINT Flags,
    UINT64 Length);

PVOID __fastcall iRequestVirtualMemory(
    PEPROCESS Process,
    UINT Flags,
    UINT64 Length);

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