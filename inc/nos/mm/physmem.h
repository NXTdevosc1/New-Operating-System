#pragma once
#include <nos/nos.h>
#include <hmapi.h>

typedef struct
{
    VMMHEADER Header;
    PEPROCESS Process;
} KPAGEHEADER;
HMIMAGE _NosHeapImages[2];
HMIMAGE *_NosPhysicalMemoryImage;
HMIMAGE *_NosKernelHeap;

// Allocates memory below 4gb
NSTATUS KRNLAPI MmAllocateLowMemory(
    UINT64 Flags, UINT64 NumPages, void **Ptr);
// Allocates memory above 4gb
NSTATUS KRNLAPI MmAllocateHighMemory(
    UINT64 Flags, UINT64 NumPages, void **Ptr);

NOS_MEMORY_DESCRIPTOR *MmCreateMemoryDescriptor(
    void *PhysicalAddress,
    UINT32 Attributes,
    UINT64 NumPages);

#define ResolvePageHeader(PhysAddr) (PageBase + ((UINT64)(PhysAddr) >> 12))