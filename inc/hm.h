/*
 * OS Optimized Heap Manager

 * This is a very fast heap manager that lets you create multiple virtual space images to be used in different scenarios
    2 of the most populars are, GPU and Process heap allocations

 * This heap manager let's you allocate from multiple processors simultanously without waiting or deadlocks

 * It also let's you share a heap with multiple images, one of the usage is that our OS Holds 2 heap images
    for the system process, one contains heaps that are below 4GB in Physical memory (32 Bit heaps)
    The other contains the 32 bit heaps include heaps that require 64 bit memory access support
    This is very useful with devices

 * This heap manager is very compact and easy to use for multiple scenarios
 *
*/

/*
   TODO : Write CPU CYCLE SCENARIOS For each function to track speed
*/

#pragma once
#include <nosdef.h>

#define HM_IMAGE_SIZE 0x1000
typedef struct _HMIMAGE HMIMAGE;

typedef void *(__fastcall *HEAP_MANAGER_CALLBACK)(HMIMAGE *Image, UINT64 Command, UINT64 Param0, UINT64 Param1);

typedef struct _PAGEHEAPDEF
{
    UINT64 Present : 1;
    UINT64 Length : 63; // 0 if end of heap
} PAGEHEAPDEF;

#ifndef HMAPI
#define HMAPI __declspec(dllimport) __fastcall
typedef struct _HMIMAGE
{
    HMIMAGE *Parent;               // Request memory from this image if not available
    HMIMAGE *DataAllocationSource; // Image from where heaps should be allocated

    UINT HeapMapping;

    UINT64 UnitLength;
    UINT64 TotalUnits;
    UINT64 UsedUnits;

    UINT64 TotalSpace;

    UINT64 BaseAddress; // In units
    UINT64 EndAddress;

    // Size bitmap (for free memory)
    UINT64 *ReservedArea;
    UINT64 ReservedAreaLength;

    void *RecentHeap;
    UINT CallbackMask;
    HEAP_MANAGER_CALLBACK Callback;
    UINT64 Rsv[0x200];

} HMIMAGE;
#endif

typedef enum
{
    HmCallbackNoMem,
    HmCallbackMapPage, // allocates and maps a zero-initialized page at specified address in param0, forced to return TRUE or crash
} HmCallbackBitmask;

typedef enum
{
    HmNoMap,
    HmPageMap, // preallocates a continuous mapping (usable for page mapping)
    HmBlockMap
} HmAccessWay;
// Returns reserved memory length
UINT64 HMAPI HeapImageCreate(
    IN OUT HMIMAGE *Image,
    IN UINT HeapMapping,
    OUT UINT64 *Commit,         // Preallocate memory
    IN UINT64 BaseUAddress,     // in units
    IN UINT64 UAddrSpaceLength, // in units
    IN UINT8 UnitLengthPw2,     // Should be in powers of 2
    IN OPT HEAP_MANAGER_CALLBACK Callback);

void HMAPI HeapImageInit(
    HMIMAGE *Image,
    void *ReservedArea,
    UINT64 InitialHeapUAddress,
    UINT InitialHeapULength);
/*
 * Local functions, avoiding synchronization, faster
 * Either use them in a single thread
 * Or use them in multiple threads with only 1 processor, and disable interrupts before calling each function
 */

PVOID HMAPI HeapAllocate(
    HMIMAGE *Image,
    UINT64 UnitCount);

BOOLEAN HMAPI HeapFree(HMIMAGE *Image, void *Ptr);

BOOLEAN HMAPI BaseHeapCreate(
    HMIMAGE *Image,
    UINT64 UnitAddress,
    UINT64 UnitCount);