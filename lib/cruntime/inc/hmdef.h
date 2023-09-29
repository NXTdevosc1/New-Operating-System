#pragma once
#include <ddk.h>
#include <simdopt.h>

#define HMAPI __declspec(dllexport) __fastcall
#include <hm.h>
#include <crt.h>

#define HM_LAST_TREE_LENGTH 130
typedef struct _hmbheap BASEHEAP;
typedef struct _hmheap
{
    UINT64 addr : 63;
    UINT64 baseheap : 1;
    UINT64 len;
} HEAPDEF;
typedef struct _hmbheap
{
    HEAPDEF def;
    UINT64 maxlen;
    struct _hmbheap *next, *prev;
} BASEHEAP;

typedef struct _HMIMAGE
{

    HMIMAGE *Parent;               // Request memory from this image if not available
    HMIMAGE *DataAllocationSource; // Image from where heaps should be allocated
    UINT8 DataAllocationLength;

    UINT HeapMapping; // eather Block headers or Address maps

    UINT64 UnitLength;

    UINT8 UnitShift;
    UINT8 UnitParentShift;

    UINT64 TotalUnits;
    UINT64 UsedUnits;

    UINT64 TotalSpace;

    UINT64 BaseAddress; // In units
    UINT64 EndAddress;

    // Size bitmap (for free memory)
    UINT64 *ReservedArea;
    UINT64 ReservedAreaLength;

    BASEHEAP *RecentHeap;
    UINT CallbackMask;
    HEAP_MANAGER_CALLBACK Callback;

    UINT64 CommitLength;

    UINT64 *AddressDirectory;
    UINT64 LenAddrDir;
    UINT64 *AddressMap;

    // Free memory (2 Exponent based sizes)
    UINT64 BaseMemoryMask;
    struct
    {
        BASEHEAP *Start;
        BASEHEAP *End;
    } BaseMemory[64];

    BASEHEAP InitialHeap;

} HMIMAGE;

#define HMINTERNAL static inline
#define HMDECL __fastcall

HMINTERNAL void HMDECL _BaseHeapPlace(HMIMAGE *Img, BASEHEAP *Heap)
{
    ULONG i;
    _BitScanReverse64(&i, Heap->def.len);
    if (_bittestandset64(&Img->BaseMemoryMask, i))
    {
        Img->BaseMemory[i].End->next = Heap;
        Heap->prev = Img->BaseMemory[i].End;
        Img->BaseMemory[i].End = Heap;
    }
    else
    {
        Img->BaseMemory[i].Start = Heap;
        Img->BaseMemory[i].End = Heap;
        Heap->prev = NULL;
    }
    Heap->next = NULL;
}

// Sorts a larger heap and allocates from it
HMINTERNAL void *HMDECL _HeapResortAllocate(HMIMAGE *Image, UINT64 UnitCount)
{
    ULONG exp;
    _BitScanReverse64(&exp, Image->BaseMemoryMask);
    BASEHEAP *h = Image->BaseMemory[exp].Start;
    while (h)
    {
        if (h->def.len >= UnitCount)
        {
            Image->RecentHeap = h;
            h->def.len -= UnitCount;
            char *p = h->def.addr << Image->UnitShift;
            h->def.addr += UnitCount;
            return p;
        }
    }
    UINT64 Len = UnitCount >> Image->UnitParentShift;
    if (UnitCount & ((1 << Image->UnitParentShift) - 1))
        Len++;
    if (HeapAllocate(Image->Parent, Len))
    {
    }
    else
    {
        // User-defined implementation
        return Image->Callback(Image, HmCallbackNoMem, UnitCount, 0);
    }
}

HMINTERNAL void HMDECL _HeapRecentRelease(HMIMAGE *Image)
{
    ULONG expmax, expcrt;
    BASEHEAP *h = Image->RecentHeap;

    _BitScanReverse64(&expmax, h->maxlen);
    _BitScanReverse64(&expcrt, h->def.len);
    if (expmax != expcrt)
    {
        if (Image->BaseMemory[expmax].Start == h && Image->BaseMemory[expmax].End == h)
        {
            _bittestandreset64(&Image->BaseMemoryMask, expmax);
        }
        else
        {
            if (Image->BaseMemory[expmax].Start == h)
            {
                Image->BaseMemory[expmax].Start = h->next;
                Image->BaseMemory[expmax].Start->prev = NULL;
            }
            else if (Image->BaseMemory[expmax].End == h)
            {
                Image->BaseMemory[expmax].End = h->prev;
                Image->BaseMemory[expmax].End->next = NULL;
            }
            else
            {
                h->next->prev = h->prev;
                h->prev->next = h->next;
            }
        }
        if (!h->def.len)
        {
            if (h->def.baseheap)
            {
                h->def.len = -1; // Remap indicator
            }
            else
            {
                // Free the subheap
                HeapFree(Image->DataAllocationSource, h);
            }
        }
        else
            _BaseHeapPlace(Image, h);
    }
    Image->RecentHeap = NULL;
}

HMINTERNAL void HMDECL _HeapSetPageEntry(HMIMAGE *Image, UINT64 Address, UINT64 Length)
{
    UINT64 DirIndex = Address >> 15, BitOff = (Address >> 9) & 0x3F;
    if (_bittestandset64(Image->AddressDirectory + DirIndex, BitOff))
    {
        PAGEHEAPDEF *Pg = (void *)(Image->AddressMap + (Address & ~0x1FF));
        Pg += Address & 0x1FF;
        Pg->Length = Length;
        Pg->Present = 1;
    }
    else
    {
        // Create and map a page
        PAGEHEAPDEF *Pg = (void *)(Image->AddressMap + (Address & ~0x1FF));
        Image->Callback(Image, HmCallbackMapPage, Pg, 0);
        Pg += Address & 0x1FF;
        Pg->Length = Length;
        Pg->Present = 1;
    }
}