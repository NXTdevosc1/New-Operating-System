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
    PVOID Buffer;
} HMUSERHEADER;
#ifndef HMAPI
#define HMAPI __declspec(dllimport) __fastcall
typedef struct _HMIMAGE
{
    HMUSERHEADER User;
    UINT64 Rsv[0x200];

} HMIMAGE;
#endif

/*
 * HM Optimized Page interface
 */
typedef struct _vmmpagehdr
{
    UINT64 Attributes : 12; // User-defined
    UINT64 Address : 52;
    struct _vmmpagehdr *Next;
    struct _vmmpagehdr *LastOrPrev;
} VMMHEADER;
// Image setup required
void HMAPI VmmCreate(HMIMAGE *Image, UINT8 NumLevels, void *Mem, UINT DescriptorSize);

PVOID HMAPI VmmAllocate(HMIMAGE *Image, UINT Level, UINT64 Count, void **Header);

#ifndef __VMMSRC
void HMAPI VmmInsert(PVOID Level,
                     PVOID Desc,
                     UINT64 Length);
void HMAPI VmmRemove(PVOID *Level,
                     PVOID Desc,
                     UINT64 Length);
BOOLEAN HMAPI VmmInstantLookup(PVOID *Level);
#define VmmLevelLength(Lvl) ((Lvl) * 4712)
#define VmmPageLevel(Image, NumLvls) ((char *)Image->User.Buffer + VmmLevelLength(NumLvls))
#endif

#pragma pack(push, 0x10)

typedef struct _HMBLK
{

    UINT64 PrevOrLast : 55;
    UINT64 PrevLength : 8;
    UINT64 Used : 1;
    UINT64 Next : 55;
    UINT64 EndingBlock : 1;
    UINT64 Length : 8;
} HMBLK, *PHMBLK;

#pragma pack(pop)

typedef PVOID(__fastcall *HMALLOCATE_ROUTINE)(HMIMAGE *Image, UINT64 PageCount);
typedef void(__fastcall *HMFREE_ROUTINE)(HMIMAGE *Image, PVOID PageStart, UINT64 PageCount);

void HMAPI oHmbInitImage(
    HMIMAGE *Image,
    HMALLOCATE_ROUTINE AllocateRoutine,
    HMFREE_ROUTINE FreeRoutine);

void HMAPI oHmbSet(HMIMAGE *Image, PHMBLK Block, UINT8 Length /*in 16 Byte blocks*/);
void HMAPI oHmbRemove(HMIMAGE *Image, PHMBLK Block, UINT8 Length);
BOOLEAN HMAPI oHmbLookup(HMIMAGE *Image);
PVOID HMAPI oHmbAllocate(
    HMIMAGE *Image,
    UINT64 Length);
BOOLEAN HMAPI oHmbFree(HMIMAGE *Image, void *Ptr);
