#pragma once
#include <nosdef.h>

/*
 * NOS KERNEL Multi-Processor Management API
*/

typedef struct _PROCESSOR PROCESSOR, *RFPROCESSOR;

enum NosMultiprocessorArchitecture{
    MpNoMultiprocessorArch,
    MpApic,
    MpSapic,
    MpXapic,
    MpX2apic,
    MpNuma,
    MpCCNuma
};

enum NosMicroProcessorArchitecture {
    ArchAmd64,
    ArchArm64
};

// This is called when processor trampoline finished its initialization sequence
typedef void (__cdecl *PROCESSOR_STARTUP_ROUTINE)(PROCESSOR* Processor, void* Context);

// PROCESSOR_CHARACTERISTICS
#define PROCESSOR_BOOTABLE 1

typedef struct _PROCESSOR_IDENTIFICATION_DATA {
    UINT64 ProcessorId;
    UINT64 Characteristics;
    PROCESSOR_STARTUP_ROUTINE StartupRoutine;
    void* Context;
    UINT64 AcpiId; // or processor index
} PROCESSOR_IDENTIFICATION_DATA;


// Processor General Utilities

/* KeRegisterProcessor :
    Registers the cpu in the kernel cpu table
    ProcessorId : Usually the APIC ID of the CPU
*/

RFPROCESSOR KRNLAPI KeRegisterProcessor(PROCESSOR_IDENTIFICATION_DATA* Identification);
RFPROCESSOR KRNLAPI KeGetProcessorById(UINT64 ProcessorId);
RFPROCESSOR KRNLAPI KeGetCurrentProcessor();
UINT64 KRNLAPI KeGetCurrentProcessorId();

void KRNLAPI KeProcessorReadIdentificationData(RFPROCESSOR Processor, PROCESSOR_IDENTIFICATION_DATA* Identification);