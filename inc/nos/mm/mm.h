#pragma once
#include <nosdef.h>
#include <nos/lock/lock.h>

typedef enum {
    ALLOCATE_BELOW_4GB = 1,
    ALLOCATE_2MB_ALIGNED_MEMORY = 2,
    ALLOCATE_1GB_ALIGNED_MEMORY = 4,
    MM_ALLOCATE_WITHOUT_DESCRIPTOR = 8 // Allocates without creating a descriptor
} ALLOCATE_PHYSICAL_MEMORY_FLAGS;



typedef struct _NOS_HEAP_TREE NOS_HEAP_TREE;
typedef struct _NOS_VIRTUAL_MEMORY_LIST NOS_VIRTUAL_MEMORY_LIST;

// HEAP Flags

typedef struct _NOS_HEAP {
    void* Address;
    UINT64 Size : 48;
    UINT64 Flags : 16; // Reserved
    struct _NOS_HEAP* Next;
} NOS_HEAP;

#define HEAP_ALIGNMENT 0x10 // Heaps are 16 byte aligned

typedef struct _NOS_HEAP_TREE_CHILD NOS_HEAP_TREE_CHILD;

typedef struct _NOS_HEAP_LIST {
    NOS_HEAP_TREE_CHILD* Parent;
    UINT EntryIndex;
    UINT64 ActiveHeaps;
    NOS_HEAP Heaps[64];
} NOS_HEAP_LIST;
typedef struct _NOS_HEAP_TREE NOS_HEAP_TREE;
typedef struct _NOS_HEAP_TREE_CHILD {
    NOS_HEAP_TREE* Parent;
    UINT EntryIndex;
    UINT64 ActiveSlots;
    UINT64 FullSlots;
    struct {
        NOS_HEAP_LIST* Child;
    } Slots[64];
} NOS_HEAP_TREE_CHILD;

typedef struct _NOS_HEAP_TREE {
    SPINLOCK ControlLock;
    volatile UINT64 ActiveSlots;
    volatile UINT64 FullSlots;
    struct {
        NOS_HEAP_TREE_CHILD* Child;
    } Slots[64];
    struct _NOS_HEAP_TREE* Next;
} NOS_HEAP_TREE;



NSTATUS KRNLAPI MmAllocatePhysicalMemory(UINT64 Flags, UINT64 NumPages, void** Ptr);
#define AllocateObject(_obj) (MmAllocatePhysicalMemory(0, ConvertToPages(sizeof(*_obj)), &_obj))
#define AllocateNextList(_nlist) AllocateObject(_nlist)
BOOLEAN KRNLAPI MmFreePhysicalMemory(
    IN void* PhysicalMemory,
    IN UINT64 NumPages
);


PVOID KRNLAPI MmAllocateMemory(
    IN PEPROCESS Process,
    IN UINT64 NumPages,
    IN UINT64 PageAttributes,
    IN UINT64 CachePolicy    
);
BOOLEAN KRNLAPI MmFreeMemory(
    IN PEPROCESS Process,
    IN void *Mem,
    IN UINT64 NumPages
);

void NOSINTERNAL KiPhysicalMemoryManagerInit();

PVOID KRNLAPI MmAllocatePool(
    UINT64 Size,
    UINT Flags
);

BOOLEAN KRNLAPI MmFreePool(
    void* Address
);

NSTATUS KRNLAPI MmShareMemory(
    IN PEPROCESS Source,
    IN PEPROCESS Destination,
    IN OUT void** VirtualAddress,
    IN UINT64 NumPages,
    IN UINT64 PageAttributes,
    IN UINT CachePolicy
);