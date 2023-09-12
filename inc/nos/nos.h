/*
 * NOS Kernel Header
 */

#pragma once

#define NOS_MAJOR_VERSION 1
#define NOS_MINOR_VERSION 0

#ifndef KRNLAPI
// Functions only accessible in kernel mode
#define KRNLAPI __declspec(dllexport) __fastcall
// NSYSAPI is both accessible in user mode or kernel mode
#define NSYSAPI __fastcall

#define IOPROTOCOL __declspec(dllexport) __stdcall

#endif

#define __TRACECALLS 1

#include <nosdef.h>
#include <nosefi.h>
#include <nos/lock/lock.h>
#include <efi/loader.h>
#include <nos/serial.h>
#include <nos/processor/processor.h>
#include <intrin.h>
#include <crt.h>
#include <nos/mm/mm.h>
#include <nos/task/process.h>
#include <nos/sys/sys.h>
#include <nos/ob/ob.h>
#include <nos/task/schedule.h>
#include <ktime.h>
#include <nos/console/console.h>
#include <nos/pnp/pnp.h>

#define DEBUG 1

#ifdef DEBUG
#define SerialLog(Msg)    \
    {                     \
        SerialWrite(Msg); \
    }
#else
#define SerialLog(Msg)
#endif

PKTIMER BestCounter;
PKTIMER BestTimeAndDateSource;

typedef NSTATUS(__cdecl *DRIVER_ENTRY)(PDRIVER Driver);

// Driver structure

typedef struct _DRIVER
{
    UINT64 DriverId;
    POBJECT ObjectHeader;
    HANDLE DriverHandle;
    UINT NumDevices;
    UINT NumIoPorts;
    PEPROCESS SystemProcess;
    DRIVER_ENTRY EntryPoint;
    void *ImageFile;
} DRIVER, *PDRIVER;

// Memory Manager Attributes
// BITMASKS
#define MM_DESCRIPTOR_ALLOCATED 1 // This descriptor represents allocated memory

// BITSETS
#define MM_DESCRIPTOR_BUSY 1 // Bit Number 1

PEPROCESS KernelProcess;

extern NOS_INITDATA *NosInitData;

void KRNLAPI KDebugPrint(IN char *Message, ...);

static inline char *KiMakeSystemNameA(char *Name, UINT16 len)
{
    char *n = MmAllocatePool(len + 1, 0);
    if (!n)
    {
        KDebugPrint("MakeSysName Failed!");
        while (1)
            __halt();
    }
    memcpy(n, Name, len + 1);
    return n;
}

static inline UINT16 *KiMakeSystemNameW(UINT16 *Name, UINT16 len)
{
    UINT16 *n = MmAllocatePool((len + 1) << 1, 0);
    if (!n)
    {
        KDebugPrint("MakeSysName Failed!");
        while (1)
            __halt();
    }
    memcpy(n, Name, (len + 1) << 1);
    return n;
}

void KiInitStandardSubsystems();

static char *InitErrors[] = {
    "Process management initialization failed."};

#define RaiseInitError(ErrNum)         \
    {                                  \
        SerialLog(InitErrors[ErrNum]); \
        while (1)                      \
            __halt();                  \
    }

NSTATUS __resevt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 DesiredAccess);

typedef volatile struct _KTIMER
{
    PDEVICE Device;
    POBJECT TimerObject;
    UINT Usage;
    UINT64 Frequency; // in HZ
    UINT64 TickCounter;
    HANDLE KernelHandle;
} KTIMER, *PKTIMER;