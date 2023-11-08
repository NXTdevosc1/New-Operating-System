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

typedef struct _HMHEADER
{
    UINT64 Address : 63;
    UINT64 FirstBlock : 1;
    struct _HMHEADER *NextBlock;
    struct _HMHEADER *PrevOrLastBlock;
} HMHEADER, *PHMHEADER;
typedef struct _HMUSERHEADER
{
    struct
    {
        PHMHEADER Block;
        UINT64 RemainingLength;
        UINT64 StartupLength;
    } BestHeap;

    UINT64 TotalMemory; // in units
    UINT64 AllocatedMemory;
    HMIMAGE *HigherImage;
    UINT64 Alignment;
    UINT64 AlignShift;
} HMUSERHEADER;
#ifndef HMAPI
#define HMAPI __declspec(dllimport) __fastcall
typedef struct _HMIMAGE
{
    HMUSERHEADER User;
    UINT64 Rsv[0x300];

} HMIMAGE;
#endif

/*
 * HM Optimized Page interface
 */

// Image setup required
UINT64 HMAPI oHmpCreateImage(
    HMIMAGE *as,
    UINT64 Alignment,
    UINT64 MaxLengthInAlignedUnits);
void HMAPI oHmpInitImage(
    HMIMAGE *as, // address space
    char *Mem);

// Request heap by specific length
PHMHEADER HMAPI oHmpGet(HMIMAGE *as, UINT64 TheoriticalLength);
// Set heap and specify it's length
void HMAPI oHmpSet(HMIMAGE *as, HMHEADER *Mem, UINT64 TheoriticalLength);
// Delete heap and specify it's length
void HMAPI oHmpDelete(HMIMAGE *as, HMHEADER *Mem, UINT64 TheoriticalLength);
// Search for the largest heap possible
// if a larger heap is found, then TRUE is returned
BOOLEAN HMAPI oHmpLookup(HMIMAGE *as);
/*
 * HM Optimized heap blocks interface
 * Doesn't require initialization
 */

/*
 * Set Image.User.AllocateFrom to request pages when there is no memory
 */

#pragma pack(push, 0x10)

typedef struct _HMBLK
{
    UINT64 Addr : 63;
    UINT64 MainBlk : 1;

    UINT64 Length;

    struct _HMBLK *Next;

    struct _HMBLK *PrevOrLast;
} HMBLK, *PHMBLK;

#pragma pack(pop)

void HMAPI oHmbSet(HMIMAGE *Image, PHMBLK Block, UINT8 Length /*in 16 Byte blocks*/);
void HMAPI oHmbRemove(HMIMAGE *Image, PHMBLK Block, UINT8 Length);
PHMBLK HMAPI oHmbLookup(HMIMAGE *Image);
PVOID HMAPI oHmbAllocate(
    HMIMAGE *Image,
    UINT64 Length);
BOOLEAN HMAPI oHmbFree(HMIMAGE *Image, void *Ptr);
