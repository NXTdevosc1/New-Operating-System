/*
NOS Driver Developement Kit
*/

#pragma once
#include <nosdef.h>
#include <ktime.h>
#include <ob.h>
#include <nosio.h>
#include <pnp.h>
#include <intmgr.h>
#include <mpmgr.h>
#include <ktask.h>
#include <kevent.h>

void KRNLAPI KDebugPrint(IN const char* Message, ...);
#define NOSAPI __cdecl
typedef NSTATUS(NOSAPI *DRIVER_ENTRY)(PDRIVER Driver);

typedef struct _DRIVER {
    UINT64 DriverId;
    void* ObjectHeader;
    HANDLE DriverHandle;
    UINT NumDevices;
    UINT NumIoPorts;
    PEPROCESS SystemProcess;
    DRIVER_ENTRY EntryPoint;
} DRIVER, *PDRIVER;



#define SYSTEM_SPURIOUS_INTERRUPT 0xFF

typedef struct _PROCESSOR PROCESSOR;



// Memory Management (PAGING)
typedef enum _PageCachePolicy{
    PAGE_CACHE_WRITE_BACK,
    PAGE_CACHE_WRITE_COMBINE,
    PAGE_CACHE_DISABLE,
    PAGE_CACHE_WRITE_PROTECT,
    PAGE_CACHE_WRITE_THROUGH
} PageCachePolicy;

// PAGE MAP FLAGS
#define PAGE_WRITE_ACCESS 2
// Enable code execution in the page
#define PAGE_EXECUTE ((UINT64)1 << 63)
#define PAGE_GLOBAL ((UINT64)1 << 8)
#define PAGE_USER 4
#define PAGE_2MB ((UINT64)1 << 7)
#define PAGE_HALFPTR ((UINT64)0x8000000)

NSTATUS KRNLAPI KeMapVirtualMemory(
    void* Process,
    IN void* _PhysicalAddress,
    IN void* _VirtualAddress,
    IN UINT64 NumPages,
    IN UINT64 PageFlags,
    IN UINT CachePolicy
);

NSTATUS KRNLAPI KeUnmapVirtualMemory(
    IN void* Process,
    IN void* _VirtualAddress,
    IN OUT UINT64* _NumPages // Returns num pages left
);

BOOLEAN KRNLAPI KeCheckMemoryAccess(
    IN void* Process,
    IN void* _VirtualAddress,
    IN UINT64 NumBytes,
    IN OPT UINT64* _Flags
);

PVOID KRNLAPI KeConvertPointer(
    IN void* Process,
    IN void* VirtualAddress
);

BOOLEAN KRNLAPI KeQueryInterruptInformation(UINT Irq, PIM_INTERRUPT_INFORMATION InterruptInformation);

typedef enum _PROCESS_CONTROL_FLAGS {
    PROCESS_MANAGE_ADDRESS_SPACE = 1
} PROCESS_CONTROL_FLAGS;

// KeReleaseControlFlag(Process, PROCESS_CONTROL_MANAGE_ADDRESS_SPACE)
PVOID KRNLAPI KeFindAvailableAddressSpace(
    IN PEPROCESS Process,
    IN UINT64 NumPages,
    IN void* VirtualStart,
    IN void* VirtualEnd,
    IN UINT64 PageAttributes
);

NSTATUS KRNLAPI KeAcquireControlFlag(IN PEPROCESS Process, IN UINT64 ControlBit);
NSTATUS KRNLAPI KeReleaseControlFlag(IN PEPROCESS Process, IN UINT64 ControlBit);


typedef enum {
    ALLOCATE_BELOW_4GB = 1,
    ALLOCATE_2MB_ALIGNED_MEMORY = 2,
    ALLOCATE_1GB_ALIGNED_MEMORY = 4,
    MM_ALLOCATE_WITHOUT_DESCRIPTOR = 8 // Allocates without creating a descriptor
} ALLOCATE_PHYSICAL_MEMORY_FLAGS;


// Memory Management (Heaps)

NSTATUS KRNLAPI MmAllocatePhysicalMemory(
    IN UINT64 Flags,
    IN UINT64 NumberOfPages,
    IN void** Address
);

BOOLEAN KRNLAPI MmFreePhysicalMemory(
    IN void* PhysicalMemory,
    IN UINT64 NumberOfPages
);

PVOID KRNLAPI MmAllocateMemory(
    IN PEPROCESS Process,
    IN UINT64 NumPages,
    IN UINT64 PageAttributes,
    IN UINT64 CachePolicy    
);

BOOLEAN KRNLAPI MmFreeMemory(
    IN PEPROCESS Process,
    IN void* Mem,
    IN UINT64 NumPages
);

PVOID KRNLAPI MmAllocatePool(
    UINT64 Size,
    UINT Flags
);

static inline PVOID __fastcall AllocateNullPool(UINT64 _Size) 
{
    PVOID v = MmAllocatePool(_Size, 0);
    if(v) {
        _Memset128A_32(v, 0, AlignForward(_Size, 0x10) >> 4);
    }
    return v;
}

BOOLEAN KRNLAPI MmFreePool(
    void* Address
);



// Locks/Semaphores
typedef __declspec(align(0x10)) UINT64 SEMAPHORE;
typedef __declspec(align(0x10)) UINT32 SPINLOCK;

void KRNLAPI ExInitSemaphore(SEMAPHORE* Semaphore, UINT MaxSlots);
BOOLEAN KRNLAPI ExSemaphoreWait(SEMAPHORE* Semaphore, UINT64 TimeoutInMs);
BOOLEAN KRNLAPI ExSemaphoreSignal(SEMAPHORE* Semaphore);


UINT64 KRNLAPI ExAcquireSpinLock(SPINLOCK* SpinLock);
void KRNLAPI ExReleaseSpinLock(SPINLOCK* SpinLock, UINT64 CpuFlags);

// Processor Management

// Firmware etc..

// Use GUIDs defined by the EFI Spec
PVOID KRNLAPI KeFindSystemFirmwareTable(
    IN char* FirmwareTableId,
    IN OPT GUID* EfiFirmwareTableGuid
);


// SDDK Library Exports

#ifndef SDDKAPI
#define SDDKAPI __declspec(dllimport) __fastcall
#endif
