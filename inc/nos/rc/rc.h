/*
 * Instead of Legacy IPC Mechanisms
 * in NOS We introduce a new RC Mechanism (Remote Communication)
 * This mechanism makes it easier to connect devices accross networks,
 * or devices connected to the computer through usb...
*/
#pragma once
#include <nosdef.h>
#include <ob.h>

// Special RC Accesses
#define RCACCESS_LISTEN 1
#define RCACCESS_MANAGE_PORT 2
#define RCACCESS_ASYNC 4
#define RCACCESS_ABSTRACT 8


typedef struct _RCHEADER {
    PETHREAD Source;
    UINT64 Message;
    void* Data;
    UINT64 Length;
    UINT Flags;
} RCHEADER, *PRCHEADER;


typedef struct _RCPORT {
    UINT64 Characteristics;
    POBJECT ObjectHeader;
    UINT NumEndpoints; // Processes that are connected to the port
    UINT NumListeners; // Processes that can listen on the port (Should be > NumEndpoints)
    PETHREAD Polling;
    PDRIVER Driver;
    RCHEADER CurrentMessage;
} RCPORT, *PRCPORT;


typedef struct _RCCONNECTION {
    POBJECT ObjectHeader;
    HANDLE Port;
    UINT64 Access;
    PETHREAD 
} RCCONNECTION, *PRCCONNECTION;

// Kernel mode functions
PRCPORT KRNLAPI RcKernelCreatePort(PDRIVER Driver, UINT64 Characteristics, char* PortName);

// Host, can either be a process, a device connection or a driver
// Transfer length should be in pages
HANDLE KRNLAPI RcConnectPort(HANDLE Host, char* PortName, UINT64 PortAccess);
void RcDisconnect(HANDLE Port);

// RC Flags
#define RC_ASYNC 1

// BOOLEAN Completed used on async requests
NSTATUS KRNLAPI RcSend(HANDLE Connection, UINT Flags, void* Data, UINT64 SizeOfData);
NSTATUS KRNLAPI RcAsyncSend(HANDLE Connection, UINT Flags, void* Data, UINT64 SizeOfData, BOOLEAN* Compeleted);
BOOLEAN KRNLAPI RcReceive(HANDLE Connection, PRCHEADER RcHeader);