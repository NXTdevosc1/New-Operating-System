#include <nos/nos.h>

#include <nos/mm/mm.h>
#include <nos/mm/physmem.h>

typedef struct
{
    UINT Flags;
    // UINT64 Length;
} VMEMBLOCK;

PVOID __fastcall __staticRetNull()
{
    return NULL;
}

void __forceinline __fastcall __FillVirtualMemory(HMIMAGE *Vmi, char *Address, UINT Lvl, UINT64 Count)
{
    if (!Count)
        return;
    const UINT64 Length = Count & 0x1FF;
    if (Length)
    {
        KPAGEHEADER *Hdr = KlAllocatePool(sizeof(KPAGEHEADER), 0);
        Hdr->Header.Address = (UINT64)Address; // Already bitshifted >> 12
        Hdr->Length = Length;
        VmmInsert(VmmPageLevel(Vmi, Lvl), Hdr, Hdr->Length);
        KDebugPrint("Inserted Fill ADDR %x LVL %d Len %x", Address, Lvl, Hdr->Length);
    }
    __FillVirtualMemory(Vmi, Address + (Length << (9 * Lvl)), Lvl + 1, Count >> 9);
}

void InitVirtualMemoryManager(PEPROCESS Process)
{

    KDebugPrint("INITVM %x PID %d", Process, Process->ProcessId);

    Process->VmImage = KRequestMemory(REQUEST_PHYSICAL_MEMORY, Process, MEM_AUTO, ConvertToPages(sizeof(HMIMAGE) + VmmLevelLength(4)));
    KDebugPrint("VMIMG %x", Process->VmImage);
    ZeroMemory(Process->VmImage, sizeof(HMIMAGE) + VmmLevelLength(4));
    VmmCreate(Process->VmImage, 4, Process->VmImage + 1, sizeof(KPAGEHEADER));
    __FillVirtualMemory(Process->VmImage, (char *)((UINT64)Process->VmSearchStart >> 12), 0, ((UINT64)Process->VmSearchEnd - (UINT64)Process->VmSearchStart) >> 12);
    if (Process->Subsystem != SUBSYSTEM_NATIVE)
    {
        // Create a new user page table
        // TODO : Copy page table of the kernel
    }
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
        if ((ret = VmmAllocate(Process->VmImage, i, Count, &Hdr)) != VMM_NOMEMORY)
        {
            KPAGEHEADER *Page = *Hdr;
            Page->Length -= Count;
            if (!Page->Length)
            {
                KDebugPrint("A page descriptor %x should be freed", Page);
                KlFreePool(Page);
            }
            else
                Page->Header.Address += (Count << (9 * i));

            if (i != PageSz)
            {
                UINT64 Remaining = (Count << (9 * (i - PageSz))) - (Length);
                __FillVirtualMemory(Process->VmImage, (char *)ret + (Length << (9 * PageSz)), PageSz, Remaining);
            }
            return ret;
        }
    }
    KDebugPrint("VALLOC_FAILED len %d PSZ %d", Length, PageSz);

    return NULL;
}
void KRNLAPI iFreeVirtualMemory(PEPROCESS Process, PVOID Address, UINT PageLength, UINT64 PageCount)
{
    __FillVirtualMemory(Process->VmImage, ((char *)((UINT64)Address >> 12)), PageLength, PageCount);
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
