/*
 * This heap allocator permits : Simultanous heap allocation and freeing between multiple processors
 * Extremely fast and easy setup
 * Sharing between two Heap images
 */
#define __CRT_SRC
#include <hmdef.h>
#include <crt.h>
#include <intrin.h>

// Returns reserved memory length
UINT64 HMAPI HeapImageCreate(
    IN OUT HMIMAGE *Image,
    IN UINT HeapMapping,
    OUT UINT64 *Commit,     // Preallocate memory
    IN UINT64 BaseUAddress, // in units
    IN UINT64 EndUAddress,  // in units
    IN UINT64 UnitLength,   // Should be in powers of 2
    IN OPT UINT CallbackMask,
    IN OPT HEAP_MANAGER_CALLBACK Callback)
{
    ObjZeroMemory(Image);
    Image->HeapMapping = HeapMapping;
    Image->BaseAddress = BaseUAddress;
    Image->EndAddress = EndUAddress;
    Image->UnitLength = UnitLength;
    _BitScanReverse64((ULONG *)&Image->UnitShift, UnitLength);
    Image->TotalSpace = EndUAddress - BaseUAddress;
    *Commit = 0;

    if (HeapMapping == HmPageMap || HeapMapping == HmBlockMap)
    {
        // Use 1 Page bitmap directory + chain of page size entries

        // Page directory length
        *Commit = ((Image->TotalSpace / 0x200000) + 0x1000) >> 12;

        if (HeapMapping == HmBlockMap)
        {
            Image->ReservedAreaLength = (*Commit) + (AlignForward((Image->TotalSpace / 0x8000), 0x1000) >> 12);
        }
        else
        {
            Image->ReservedAreaLength = *Commit + (AlignForward((Image->TotalSpace / 0x200), 0x1000) >> 12);
        }
    }

    Image->CommitLength = *Commit;

    Image->RecentHeap = &Image->InitialHeap;

    return Image->ReservedAreaLength;
}

void HMAPI HeapImageInit(
    HMIMAGE *Image,
    void *ReservedArea,
    UINT64 InitialHeapUAddress,
    UINT InitialHeapULength)
{
    Image->ReservedArea = ReservedArea;
    XmemsetAligned(ReservedArea, 0, Image->CommitLength * 32);
    Image->InitialHeap.def.addr = InitialHeapUAddress;
    Image->InitialHeap.def.baseheap = 1;
    Image->InitialHeap.def.len = InitialHeapULength;
    Image->InitialHeap.maxlen = InitialHeapULength;

    _BaseHeapPlace(Image, &Image->InitialHeap);
}

void *HMAPI HeapBasicAllocate(
    HMIMAGE *Image,
    UINT64 UnitCount)
{
    if (!Image->RecentHeap)
        return NULL;
    if (Image->RecentHeap->def.len < UnitCount)
    {
        _HeapRecentRelease(Image);
        return _HeapResortAllocate(Image, UnitCount);
    }
    Image->RecentHeap->def.len -= UnitCount;
    char *p = Image->RecentHeap->def.addr << Image->UnitShift;
    Image->RecentHeap->def.addr += UnitCount;
    return p;
}

void *HMAPI HeapAllocate(
    HMIMAGE *Image,
    UINT64 UnitCount)
{
    void *p = HeapBasicAllocate(Image, UnitCount);
    return p;
}

BOOLEAN HMAPI HeapFree(HMIMAGE *Image, void *Ptr)
{
}

BOOLEAN HMAPI BaseHeapCreate(
    HMIMAGE *Image,
    UINT64 UnitAddress,
    UINT64 UnitCount)
{
    BASEHEAP *h = HeapBasicAllocate(Image->DataAllocationSource, Image->DataAllocationLength);
    if (!h)
        return FALSE;
    h->def.addr = UnitAddress;
    h->def.len = UnitCount;
    h->def.baseheap = 1;
    h->maxlen = h->def.len;

    _BaseHeapPlace(Image, h);
    return TRUE;
}