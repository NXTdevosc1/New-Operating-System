#pragma once
#include <ddk.h>
#include <eodxsys.h>
#include <pnp.h>
#include <intrin.h>
// From lock.h
typedef volatile struct _MUTEX {
    void* Owner; // THREAD_OBJECT
    UINT32 AccessLock;
} MUTEX;
#include <efi/loader.h>


#define EODX_NUMFUNCTIONS 1


typedef struct _EODXIOENTRY {
    UINT64 PrivilegeMask; // will be used later
    void* Callback;
    UINT8 NumArgs;
} EODXIOENTRY;

EODXIOENTRY EodxIo[EODX_NUMFUNCTIONS];

typedef struct _EODX_ADAPTER EODX_ADAPTER, *PEODX_ADAPTER;

typedef NSTATUS (__fastcall* EODX_ADAPTER_CALLBACK)(PEODX_ADAPTER Adapter, void* Context, UINT Command, PVOID Payload);
typedef struct _EODX_ADAPTER {
    UINT64 Id;
    UINT16 Version;
    UINT16 AdapterName[121];
    EODX_ADAPTER_CALLBACK Callback;
    void* Context;
    PEODX_ADAPTER Previous, Next;
} EODX_ADAPTER, *PEODX_ADAPTER;


// ADAPTER_CHARACTERISTICS
#define ADAPTER_SOFTWARE 1

PEODX_ADAPTER iEodxCreateDisplayAdapter(
    IN UINT16* AdapterName,
    IN UINT64 Characteristics, 
    IN UINT16 Version,
    IN OPT void* Context,
    IN EODX_ADAPTER_CALLBACK AdapterCallback
);

PEODX_ADAPTER iEodxInitSoftwareGraphicsProcessor();
