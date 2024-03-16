#pragma once
#include <nosdef.h>
#include <hmapi.h>
// #define MEM_2MB 2
// #define MEM_1GB 4
// #define MEM_READ_ONLY 8
// #define MEM_EXECUTE 0x10
// #define MEM_4KB 0x20

#define LARGEPAGE (0x200)
#define HUGEPAGE (0x200 * 0x200)
typedef enum
{
    NormalPageSize = 0,
    LargePageSize,
    HugePageSize,
    MaxPageSize = HugePageSize
} KPageSize;

typedef enum
{
    REQUEST_PHYSICAL_MEMORY,
    REQUEST_VIRTUAL_MEMORY,
} KMemType;

// Flags
#define MEM_AUTO 0          // Read-write pages with automatic page size
#define MEM_SMALL_PAGES 0x1 // 4KB
#define MEM_LARGE_PAGES 0x2 // 2MB
#define MEM_HUGE_PAGES 0x4  // 1GB
#define MEM_READONLY 0x8
#define MEM_EXECUTE 0x10
#define MEM_GLOBAL 0x20
#define MEM_WRITECOMBINE 0x40
#define MEM_WRITE_THROUGH 0x80
#define MEM_COMMIT 0x100

PVOID KRNLAPI
KRequestMemory(
    UINT RequestType,
    PEPROCESS Process,
    UINT Flags,
    UINT64 Length);

PVOID KRNLAPI KFreeMemory(
    PVOID Address,
    UINT64 Length,
    UINT Type);

// PVOID KRNLAPI MmAllocatePhysicalPages(
//     IN UINT PageSize,
//     IN UINT64 Length);

// void KRNLAPI MmFreePhysicalPages(
//     IN PVOID Address,
//     IN UINT64 Count);

// typedef struct _VMEMDESC
// {
//     VMMHEADER Header;
//     // UINT Attributes;

// } VIRTUAL_MEMORY_DESCRIPTOR;