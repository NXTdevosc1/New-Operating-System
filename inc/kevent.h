#pragma once
#include <nosdef.h>



// SYSTEM EVENTS (for custum events go to our website to get an event id)


typedef enum _SYSTEM_EVENTS {
    SYSTEM_EVENT_DEVICE_ADD,
    SYSTEM_EVENT_PARTITION_ADD,
} SYSTEM_EVENTS;



typedef NSTATUS (__cdecl *SYSTEM_EVENT_HANDLER)(void* Context);

// -------------------------------------------------

BOOLEAN KRNLAPI KeRegisterEventHandler(
    UINT64 EventId,
    UINT64 Flags,
    SYSTEM_EVENT_HANDLER EventHandler
);

BOOLEAN KRNLAPI KeUnregisterEventHandler(
    UINT64 EventId,
    SYSTEM_EVENT_HANDLER EventHandler
);

NSTATUS KRNLAPI KeSignalEvent(HANDLE EventHandle, void* Context, void** EventDescriptor);
BOOLEAN KRNLAPI KeRemoveEvent(void* EventDescriptor);
HANDLE KRNLAPI KeOpenEvent(UINT64 EventId);