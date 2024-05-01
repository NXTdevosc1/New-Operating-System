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
    Major->Length = 0x200;
    VmmInsert(VmmPageLevel(Process->VmImage, 3), Major, 256);
    // TODO : Copy page table of the kernel
}

static __forceinline void __VmmFillRemainingMemory(KPAGEHEADER *Source, UINT Lvl, UINT64 Length)
{
    if (!Length)
        return;
    const UINT64 Count = Length & 0x1FF;
    KPAGEHEADER *Hdr = KlAllocatePool(sizeof(KPAGEHEADER) * 512, 0);
    Hdr->Header.Address = Source->Header.Address;
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
        KDebugPrint("valloc %d", i);
        if ((ret = VmmAllocate(Process->VmImage, i, Count, &Hdr)) != VMM_NOMEMORY)
        {
            KPAGEHEADER *Page = *Hdr;
            KDebugPrint("VALLOC_SUCCESS len %d PSZ %d FoundPSZ %d ADDR %x RemainingLen %d", Length, PageSz, i, ret, Page->Length);
            Page->Length -= Count;
            Page->Header.Address += (Count << (12 + (9 * i)));
            if (i != PageSz)
            {
                UINT64 Remaining = (Count << (9 * (i - PageSz))) - (Length);
                KDebugPrint("Remaining")
                    __MmFillRemainingPages(Page + (Length << (9 * PageSz)), PageSz, Remaining);
            }
            return ret;
        }
    }
    KDebugPrint("VALLOC_FAILED len %d PSZ %d", Length, PageSz);

    return (PVOID)(0x1234);
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
