#include <nos/mm/mm.h>
#include <nos/mm/physmem.h>

void InitVirtualMemoryManager(PEPROCESS Process, PVOID SearchStart, PVOID SearchEnd)
{
    Process->VmSearchStart = SearchStart;
    Process->VmSearchEnd = SearchEnd;
    Process->VmImage = MmAllocatePool(sizeof(HMIMAGE) + VmmLevelLength(4), 0);
    VmmCreate(Process->VmImage, 4, Process->VmImage + 1, sizeof(VIRTUAL_MEMORY_DESCRIPTOR));
    // VmmInsert()
}

PVOID KeAllocateAddress(PEPROCESS Process, UINT PageLength, UINT64 PageCount)
{
}

void KeFreeAddress(PEPROCESS Process, PVOID Address, UINT PageLength, UINT64 PageCount)
{
}

PVOID KeCommitPages(PVOID Address, UINT Attributes)
{
}

PVOID KeDecommitPages(PVOID Address, UINT Where)
{
    // De-allocate
    // Compress
    // Page File
}
