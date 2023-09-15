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

typedef struct _HMHEAP
{
    /*Everything is in units*/
    ULONG Exponent;
    UINT64 Address;
    UINT FullLength; // if > 0 then this is an initial heap
    UINT CurrentLength;
    struct _HMHEAP *Next;
    struct _HMHEAP *Previous;
} HMHEAP;
typedef BOOLEAN(__fastcall *HEAP_MANAGER_CALLBACK)(HMIMAGE *Image, UINT64 Command, UINT64 Param0, UINT64 Param1);

#ifndef HMAPI
#define HMAPI __declspec(dllimport) __fastcall
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
    UINT64 Rsv[0x200];
} HMIMAGE;
#endif

typedef enum
{
    HmCallbackFreeMemory,
    HmCallbackRequestMemory
} HmCallbackBitmask;

#define HIMAGE_NO_BLOCKHEADER 1
#define HIMAGE_SELF_ALLOCATE 2     // use the current image to allocate heaps
#define HIMAGE_COMMIT_ADDRESSMAP 4 // don't use directories for efficiency

// Returns reserved memory length
UINT64 HMAPI HmCreateImage(
    IN OUT HMIMAGE *Image,
    IN UINT Flags,
    OUT UINT64 *Commit,     // Preallocate memory
    IN UINT64 BaseUAddress, // in units
    IN UINT64 EndUAddress,  // in units
    IN UINT64 UnitLength,   // Should be in powers of 2
    IN OPT UINT CallbackMask,
    IN OPT HEAP_MANAGER_CALLBACK Callback);

void HMAPI HmInitImage(
    HMIMAGE *Image,
    void *ReservedArea,
    UINT64 InitialHeapUAddress,
    UINT InitialHeapULength);
/*
 * Local functions, avoiding synchronization, faster
 * Either use them in a single thread
 * Or use them in multiple threads with only 1 processor, and disable interrupts before calling each function
 */

void HMAPI HmLocalCreateHeap(
    IN HMIMAGE *Image,
    IN void *Address,
    IN UINT64 Length);
void HMAPI HmRecentHeapEmpty(IN HMIMAGE *Image);
PVOID HMAPI HmLocalHeapUnsufficientAlloc(IN HMIMAGE *Image, IN UINT64 Size);
// 10 Lines of code

#define HmLocalAlloc(Image, Length, Ptr)                                                          \
    {                                                                                             \
        if (Image->RecentHeap->CurrentLength < Length)                                            \
        {                                                                                         \
            Ptr = HmLocalHeapUnsufficientAlloc(Image, Length);                                    \
        }                                                                                         \
        else                                                                                      \
        {                                                                                         \
            Ptr = (PVOID)((Image->RecentHeap->Address + Image->BaseAddress) * Image->UnitLength); \
            Image->RecentHeap->Address += Length;                                                 \
            Image->RecentHeap->CurrentLength -= Length;                                           \
            Image->UsedUnits += Length;                                                           \
            if (!Image->RecentHeap->CurrentLength)                                                \
                HmRecentHeapEmpty(Image);                                                         \
        }                                                                                         \
    }

// 4 Lines of code
BOOLEAN HMAPI HmLocalFree(
    IN HMIMAGE *Image,
    IN void *Address);
