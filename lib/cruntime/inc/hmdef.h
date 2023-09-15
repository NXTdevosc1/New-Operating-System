#pragma once
#include <ddk.h>
#include <simdopt.h>

#define HMAPI __declspec(dllexport) __fastcall
#include <hm.h>
#include <crt.h>

#define HM_LAST_TREE_LENGTH 130

typedef struct _HMIMAGE
{

    UINT Flags;

    UINT64 UnitLength;
    UINT64 TotalUnits;
    UINT64 UsedUnits;

    UINT64 TotalSpace;

    UINT64 BaseAddress; // In units
    UINT64 EndAddress;

    // Size bitmap (for free memory)
    UINT64 *ReservedArea;
    UINT64 ReservedAreaLength;

    HMHEAP *RecentHeap;
    UINT CallbackMask;
    HEAP_MANAGER_CALLBACK Callback;

    UINT64 *AddressDirectory;
    UINT64 LenAddrDir;
    UINT64 *AddressMap;

    // Free memory (2 Exponent based sizes)
    UINT PresentMem;
    HMHEAP *Mem[32];
    HMHEAP *MemEnd[32];

    HMHEAP InitialHeap;

} HMIMAGE;

#define HMINTERNAL static inline
#define HMDECL __fastcall

HMINTERNAL void *HMDECL _HmTakeSpace(HMIMAGE *Image, UINT64 Units);

HMINTERNAL void *HMDECL _HmAllocateSubMap(
    HMIMAGE *Image);

HMINTERNAL void HMDECL _HmClearMem(void *Mem, UINT64 LengthInBytes);

extern inline __declspec(dllexport) void HMDECL _HmPutHeap(
    HMIMAGE *Image,
    HMHEAP *Heap,
    void *Map,
    UINT64 Key);
