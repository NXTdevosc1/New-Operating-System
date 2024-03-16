#include <nos/nos.h>

#include <nos/mm/mm.h>
#include <nos/mm/physmem.h>

typedef struct
{
    UINT Flags;
    // UINT64 Length;
} VMEMBLOCK;

void InitVirtualMemoryManager(PEPROCESS Process, PVOID SearchStart, PVOID SearchEnd)
{
    KDebugPrint("INITVM %x", Process);
    Process->VmSearchStart = SearchStart;
    Process->VmSearchEnd = SearchEnd;
    Process->VmImage = KRequestMemory(REQUEST_PHYSICAL_MEMORY, Process, MEM_AUTO, ConvertToPages(sizeof(HMIMAGE) + VmmLevelLength(4)));
    KDebugPrint("VMIMG %x", Process->VmImage);
    ZeroMemory(Process->VmImage, sizeof(HMIMAGE) + VmmLevelLength(4));
    VmmCreate(Process->VmImage, 4, Process->VmImage + 1, sizeof(KPAGEHEADER));
    KPAGEHEADER *Major = KlAllocatePool(sizeof(KPAGEHEADER), 0);
    ObjZeroMemory(Major);
    VmmInsert(VmmPageLevel(Process->VmImage, 3), Major, 256);
    // TODO : Copy page table of the kernel
}

PVOID __fastcall iRequestVirtualMemory(
    PEPROCESS Process,
    UINT Flags,
    UINT64 Length)
{
    UINT PageSz = 0;
    if (Flags & MEM_HUGE_PAGES)
        PageSz = 2;
    else if (Flags & MEM_LARGE_PAGES)
        PageSz = 1;

    UINT64 Count = Length;

    KPAGEHEADER **Hdr;
    PVOID ret;
    for (int i = PageSz; i < 4; i++, Count = AlignForward(Count, 0x200) >> 9)
    {
        if ((ret = VmmAllocate(Process->VmImage, i, Count, &Hdr)))
        {

            return ret;
        }
    }

    return (PVOID)(0x0234);
}
void KRNLAPI iFreeVirtualMemory(PEPROCESS Process, PVOID Address, UINT PageLength, UINT64 PageCount)
{
}

PVOID KCommitPages(PVOID Address, UINT Attributes)
{
}

PVOID KDecommitPages(PVOID Address, UINT Where)
{
    // De-allocate
    // Compress
    // Page File
}
