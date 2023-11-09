#pragma once
#include <nosdef.h>

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
    MaxPageSize
} KPageSize;

PVOID KRNLAPI MmRequestContiguousPages(
    UINT PageSize,
    UINT64 Length);

PVOID KRNLAPI MmRequestContiguousPagesNoDesc(
    IN UINT PageSize,
    IN OUT UINT64 *LengthRemaining,
    PVOID *SystemReserved);