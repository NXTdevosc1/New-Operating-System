#pragma once
#include <nosdef.h>

// Memory management

PVOID KRNLAPI KlAllocatePool(
    UINT64 Size,
    UINT Flags);

BOOLEAN KRNLAPI KlFreePool(
    void *Address);
