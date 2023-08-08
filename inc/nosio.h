#pragma once
#include <nosdef.h>
#include <ob.h>
/*
 * IO_CALLBACK_FLAGS
*/

#define IO_CALLBACK_SYNCHRONOUS 1

#define IO_CALLBACK_SPINLOCK 31 // This value is reserved for use by the IO Manager

typedef PVOID IORESULT;

#define IOENTRY __fastcall

#define IOPARAMS HANDLE DeviceHandle, UINT Function, UINT8 NumParameters, void** Parameters
typedef IORESULT(IOENTRY *DEVICE_IO_CALLBACK)(
    HANDLE DeviceHandle,
    UINT Function,
    UINT8 NumParameters,
    void** Parameters
);

typedef union _IOARGS {
    UINT64 ArgI;
    void* ArgV;
} IOARGS;



typedef struct _IO_INTERFACE_DESCRIPTOR {
    UINT NumFunctions;
    UINT Flags;
    DEVICE_IO_CALLBACK IoCallback;
} IO_INTERFACE_DESCRIPTOR;

BOOLEAN KRNLAPI IoSetInterface(
    PDEVICE Device,
    IO_INTERFACE_DESCRIPTOR* Io
);



IORESULT KRNLAPI IoProtocol(
    HANDLE DeviceHandle,
    UINT Function,
    UINT8 NumParameters,
    void** Parameters
);