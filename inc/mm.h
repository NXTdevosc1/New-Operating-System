#pragma once
#include <nosdef.h>
#include <hmapi.h>
#define MEM_2MB 2
#define MEM_1GB 4
#define MEM_READ_ONLY 8
#define MEM_EXECUTE 0x10
#define MEM_4KB 0x20

#define LARGEPAGE (0x200)
#define HUGEPAGE (0x200 * 0x200)
typedef enum
{
    NormalPageSize = 0,
    LargePageSize,
    HugePageSize,
    MaxPageSize = HugePageSize
} KPageSize;
PVOID KRNLAPI MmAllocatePhysicalPages(
    IN UINT PageSize,
    IN UINT64 Length);

void KRNLAPI MmFreePhysicalPages(
    IN PVOID Address,
    IN UINT64 Count);

typedef struct _VMEMDESC
{
    VMMHEADER Header;
    // UINT Attributes;

} VIRTUAL_MEMORY_DESCRIPTOR;