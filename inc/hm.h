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

#define HM_IMAGE_SIZE 0x100

#ifndef HMAPI
#define HMAPI __declspec(dllimport) __fastcall
typedef char HMIMAGE;
__declspec(dllimport) void* HmiAllocateSpace(IN HMIMAGE* Image, IN UINT64 Size, OUT void** OutHeap);
#else
typedef struct _HMIMAGE HMIMAGE;

#endif

/*
 Called when an initial heap is free at all length again
 or when current length is set to a target length mask, typically 0x1000

   returns TRUE if released
*/

#define HM_RELEASE_MEMORY 0
#define HM_REQUEST_MEMORY 1

typedef BOOLEAN (__fastcall *HEAP_MANAGER_CALLBACK)(HMIMAGE* Image, UINT Cmd, UINT64 Param);
BOOLEAN HMAPI HmCreateImage(
    IN HMIMAGE* Image,
    IN HMIMAGE* AllocateFrom, // if Set to NULL, descriptors will be allocated within the heap
    IN ULONG UnitLength,
    IN void* StartAddress, // In Bytes, aligned with unit length
    IN void* EndAddress, // In Bytes, aligned with unit length
    IN void* InitialHeapAddress, // In Bytes, aligned with units
    IN UINT64 InitialHeapLength, // In Units
    IN OPT HEAP_MANAGER_CALLBACK Callback
);

/*
 * Local functions, avoiding synchronization, faster
 * Either use them in a single thread
 * Or use them in multiple threads with only 1 processor, and disable interrupts before calling each function
*/



void HMAPI HmLocalCreateHeap(
    IN HMIMAGE* Image,
    IN void* Address,
    IN UINT64 Length
);

// 10 Lines of code
PVOID HMAPI HmLocalAllocate(
    IN HMIMAGE* Image,
    IN UINT64 Size
);

// 4 Lines of code
BOOLEAN HMAPI HmLocalFree(
    IN HMIMAGE* Image,
    IN void* Address
);
