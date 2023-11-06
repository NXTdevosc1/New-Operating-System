#include <nos/nos.h>
#include <mm.h>
#include <nos/mm/physmem.h>

PVOID KRNLAPI KeRequestContiguousPages(
    UINT PageSize,
    UINT64 Length)
{
    KDebugPrint("Looking up");
    BOOLEAN b = oHmpLookup(_NosPhysical4KBImage);
    KDebugPrint("Lookup done %s", b ? "TRUE" : "FALSE");

    KDebugPrint("Length : %x ADDR %x", _NosPhysical4KBImage->User.BestHeap.StartupLength, _NosPhysical4KBImage->User.BestHeap.Block->Address);
    return NULL;
}

void KRNLAPI KeFreePages(
    PVOID Address)
{
}