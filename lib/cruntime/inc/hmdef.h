#pragma once
#include <ddk.h>
#include <simdopt.h>

#define HMAPI __declspec(dllexport) __fastcall
#include <hm.h>
#include <crt.h>

#define HM_LAST_TREE_LENGTH 130

typedef struct _HMIMAGE HMIMAGE;
typedef struct _HMHEAP HMHEAP;
typedef volatile struct _HMTREEHEAD HMTREEHEAD;

#pragma pack(push, 1)

typedef struct _HMHEAP
{
    UINT64 Address : 48; // In units
    UINT64 Dynamic : 48; // In units

    /*
    For first heap : pointer to last heap
    For other heaps : pointer to previous heaps

    */
} HMHEAP;

#pragma pack(pop)

typedef struct _HMIMAGE
{

    UINT64 UnitLength;
    UINT64 TotalUnits;
    UINT64 UsedUnits;

    UINT UnitShift;
    BOOLEAN EnableBlockHeader; // Includes heap with the block chunk, Unit length is required to be 32 bytes

    UINT64 AddressLevels; // Last level points to a page of heaps
    UINT64 SizeLevels;

    UINT64 BaseAddress; // In Units
    UINT64 EndAddress;  // In Units

    UINT64 *SizeMap; // 1 bit per entry, last level maps a single page
    UINT64 SzMapLength;
    UINT64 *AddressMap; // 2 bits per entry, 128 Bit entries
    UINT64 AddrMapLength;

    HMHEAP InitialHeap;
    HMHEAP *RecentHeap;

    UINT64 CallbackMask;
    HEAP_MANAGER_CALLBACK Callback;

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
