#pragma once
#include <ddk.h>
#include <intrin.h>
#include <immintrin.h>
#include <zmmintrin.h>

#define HMAPI __declspec(dllexport) __fastcall
#include <hm.h>
#include <crt.h>


typedef struct _HMIMAGE HMIMAGE;
typedef struct _HMHEAP HMHEAP;
typedef volatile struct _HMTREEHEAD HMTREEHEAD;

typedef struct _HMHEAP {
    UINT64 Address, Length;
    HMHEAP* Next, *Previous; // used to reform the initial heap

    /*
     * in Allocated Memory Heap :
       * If AllocateFrom == NULL, This is the pointers that continues the list
    */
    // HMHEAP* SecondaryPtr;
    // HMHEAP* ThirdPtr;
} HMHEAP;

typedef struct _HMIMAGE {
    __m256i Xlvlbitshift;
    __m256d Xlvlbitmask;

// Keeping track of units
    UINT64 TotalUnits;
    UINT64 UsedUnits;

    // In this way we get rid of division which is over 100 times slower than multiplication
    ULONG UnitLength;
    ULONG LengthShift;

// Image settings
    /* AllocateFrom:
     * Typically a pointer to the image used by AllocatePool() function for the current processor
     * if NULL then the descriptor will be allocated within the heap
    */
    HMIMAGE* AllocateFrom;
    UINT64 DescriptorSize; // In units
    UINT64 StartAddress;
    UINT64 EndAddress;
    UINT64 AddressSpaceLength;

// Debugging
    UINT64 CurrentMemoryUsage; // Used memory by the heap manager

// Optimization bitshifts & masks
    ULONG FirstLvlBitmask, SubLvlBitmask;
    ULONG FirstLvlBitshift, SecondLvlBitshift, ThirdLvlBitshift;


// Heaps
    UINT SubLevelEntries;
    UINT FirstLevelEntries; // Number of entries (or bits) represented in the first level of the tree

    UINT64 FirstLevelLength;
    UINT64 SubLevelLength;


    void* AddressMap; // if AllocateFrom != NULL then this is a HMTREEHEAD* Otherwise it is a list of heaps
    void* SizeMap; // Dynamic size bitmap, followed by pointers


    void* AllocatedEnd; // Unusable if AllocateFrom != NULL

    HMHEAP* RecentHeap;


    HMHEAP InitialHeap;

    HEAP_MANAGER_CALLBACK HeapEventCallback;

} HMIMAGE;

typedef struct {
    UINT16 Least;
    UINT16 Last;
    UINT64 Bitmap[];
} HMTREEHEADER;

#define HMINTERNAL static inline
#define HMDECL __fastcall

char _format[];

static inline void Log(char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf_s(_format, 0x1FF, fmt, args);
    va_end(args);
    char* f = _format;
    for(;*f;f++) {
        __outbyte(0x3F8, *f);
    }
    __outbyte(0x3F8, '\n');
}

HMINTERNAL void* HMDECL _HmTakeSpace(HMIMAGE* Image, UINT64 Units);
HMINTERNAL void* HMDECL _HmAllocateMap(
    HMIMAGE* Image,
    ULONG NumEntries
);

HMINTERNAL void HMDECL _HmClearMem(void* Mem, UINT64 LengthInBytes);

HMINTERNAL void HMDECL _HmPutHeap(
    HMIMAGE* Image,
    HMHEAP* Heap,
    void* Map,
    UINT64 Key
);