#pragma once
#include <nosdef.h>
#include <nosio.h>
#define __NOSKXPNP

#include <pnp.h>

typedef struct _DEVICE {
    SPINLOCK SpinLock;
    POBJECT ObjectDescriptor;

    
    UINT DeviceType;
    UINT Bus;
    UINT64 DeviceCharacteristics;
    void* Context;

    UINT16* DisplayName;
    UINT16 DisplayNameLength;


    IO_INTERFACE_DESCRIPTOR Io;
} DEVICE, *PDEVICE;