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

#pragma pack(push, 1)

typedef struct {
    UINT16 Least;
    UINT64 Bitmap[];
} HMMAP, *PHMMAP;

#pragma pack(pop)

typedef struct _HMIMAGE {
// Used to sort a key to an index in the Heap Address space
    UINT32 FirstLvlBitmask, SubLvlBitmask, slb1, slb2;
    UINT32 FirstLvlBitshift, SecondLvlBitshift, ThirdLvlBitshift, ignore0;

    

// Keeping track of units
    UINT64 TotalUnits;
    UINT64 UsedUnits;
    ULONG UnitLength;
    ULONG UnitLengthShift;

    // In this way we get rid of division which is over 100 times slower than multiplication

// Image settings
    HMIMAGE* AllocateFrom; // Used to allocate heap descriptor
    UINT uDescriptorSize; // In units
    
    BOOLEAN InsertBlockHeader; // if FALSE then free can be used on whatever address, and it might be dangerous
    UINT BlockHeaderSize;


    UINT64 MaxLength; // Max size of a heap in units, max is 2^40
    ULONG LastBit;


// Heaps
    UINT SubLevelEntries;
    UINT FirstLevelEntries; // Number of entries (or bits) represented in the first level of the tree
    

    UINT FirstLevelLength;
    UINT SubLevelLength;



    PHMMAP SizeMapStart; // Dynamic size bitmap, followed by pointers
    UINT SizeMapLength;

    HMHEAP* BlockHeaderStart;
    HMHEAP* BlockHeaderEnd;

    HMHEAP* RecentHeap;


    HMHEAP InitialHeap;

    UINT64 CallbackMask;
    HEAP_MANAGER_CALLBACK Callback;

} HMIMAGE;



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


HMINTERNAL void* HMDECL _HmAllocateSubMap(
    HMIMAGE* Image
);

HMINTERNAL void HMDECL _HmClearMem(void* Mem, UINT64 LengthInBytes);

extern inline __declspec(dllexport) void HMDECL _HmPutHeap(
    HMIMAGE* Image,
    HMHEAP* Heap,
    void* Map,
    UINT64 Key
);

