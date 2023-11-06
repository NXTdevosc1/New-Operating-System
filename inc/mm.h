#pragma once
#include <nosdef.h>

#define MEM_2MB 2
#define MEM_1GB 4
#define MEM_READ_ONLY 8
#define MEM_EXECUTE 0x10
#define MEM_4KB 0x20

PVOID KRNLAPI KeRequestContiguousPages(
    UINT PageSize,
    UINT64 Length);