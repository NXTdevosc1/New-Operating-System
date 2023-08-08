#pragma once
#include <nosdef.h>

#define GetCurrentPageTable() ((void*)(__readcr3() & ~0xFFF))

/*

PAT Access respectively using : PWT, PCD, PAT

PAT ENCODINGS :

PAT_WRITE_BACK 000,
PAT_WRITE_COMBINING 001,
PAT_UNCACHEABLE 010,
PAT_WRITE_PROTECT 011,
PAT_WRITE_THROUGH 100,
PAT_WRITE_BACK 101,
PAT_WRITE_BACK 110,
PAT_WRITE_BACK 111
*/

#define PAGE_WRITE_ACCESS 2
// Enable code execution in the page
#define PAGE_EXECUTE ((UINT64)1 << 63)
#define PAGE_GLOBAL ((UINT64)1 << 8)
#define PAGE_USER 4
#define PAGE_2MB ((UINT64)1 << 7)

// Default is write back
typedef enum _PageCachePolicy{
    PAGE_CACHE_WRITE_BACK,
    PAGE_CACHE_WRITE_COMBINE,
    PAGE_CACHE_DISABLE,
    PAGE_CACHE_WRITE_PROTECT,
    PAGE_CACHE_WRITE_THROUGH
} PageCachePolicy;

// Processor Memory Management Utilities

NSTATUS KRNLAPI HwMapVirtualMemory(
    IN void* PageTable,
    IN void* _PhysicalAddress,
    IN void* _VirtualAddress,
    IN UINT64 NumPages,
    IN UINT64 PageFlags,
    IN UINT CachePolicy
);

NSTATUS KRNLAPI KeMapVirtualMemory(
    IN PEPROCESS Process,
    IN void* _PhysicalAddress,
    IN void* _VirtualAddress,
    IN UINT64 NumPages,
    IN UINT64 PageFlags,
    IN UINT CachePolicy
);

NSTATUS KRNLAPI KeUnmapVirtualMemory(
    IN PEPROCESS Process,
    IN void* _VirtualAddress,
    IN OUT UINT64* _NumPages // Returns num pages left
);

BOOLEAN KRNLAPI KeCheckMemoryAccess(
    IN PEPROCESS Process,
    IN void* _VirtualAddress,
    IN UINT64 NumBytes,
    IN OPT UINT64* _Flags
);

PVOID KRNLAPI KeConvertPointer(
    IN PEPROCESS Process,
    IN void* VirtualAddress
);

PVOID KRNLAPI KeFindAvailableAddressSpace(
    IN PEPROCESS Process,
    IN UINT64 NumPages,
    IN void* VirtualStart,
    IN void* VirtualEnd,
    IN UINT64 PageAttributes
);


PVOID KRNLAPI HwFindAvailableAddressSpace(
    IN void* AddressSpace,
    IN UINT64 NumPages,
    IN void* VirtualStart,
    IN void* VirtualEnd,
    IN UINT64 PageAttributes
);
