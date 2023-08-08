/*
- processor.h

This header is included by the nos.h main header
*/

#pragma once
#include <nosdef.h>
#include <nos/lock/lock.h>
#include <intmgr.h>
#include <nos/processor/internal.h>
#include <nos/processor/paging.h>
#include <mpmgr.h>


#define MAX_PROCESSOR_NAME_LENGTH 120

typedef struct _PROCESSOR PROCESSOR, *RFPROCESSOR;

void* LocalApicAddress;

static inline UINT32 ApicRead(UINT Offset) {
    return *(volatile UINT32*)((char*)LocalApicAddress + Offset);
}

static inline void ApicWrite(UINT Offset, UINT Value) {
    *(volatile UINT32*)((char*)LocalApicAddress + Offset) = Value;
}

#define CurrentApicId() (ApicRead(0x20) >> 24)

// Used by scheduler
void CpuEnableApicTimer();
void CpuDisableApicTimer();


volatile UINT64 NumProcessors;




typedef struct _INTERRUPT_DESCRIPTOR {
    INTERRUPT_SERVICE_HANDLER Handler;
    void* Context;
} INTERRUPT_DESCRIPTOR;


// 188 Interrupt vectors for each cpu (INTERRUPTS 32-220)
// Each interrupt can have 64 Handlers
typedef struct _INTERRUPT_ARRAY {
    struct {
        UINT Irq;
        SPINLOCK SpinLock;
        UINT64 Present;
        INTERRUPT_DESCRIPTOR Descriptors[64];
    } Interrupts[188];
} INTERRUPT_ARRAY;

typedef struct _PROCESSOR_INTERNAL_DATA {
    // Scheduler data (8 Byte aligned fields)
    BOOLEAN SchedulingEnabled; // 0x00
    UINT32 RemainingBurst; // 0x04

    PETHREAD CurrentThread; // 0x08
    PETHREAD IdleThread; // 0x10
    PETHREAD NextThread; // 0x18

    PROCESSOR* Processor; // 0x20
    UINT64 SchedulingTimerIv; // 0x28
    UINT64 SchedulingTimerFrequency; // 0x30
} PROCESSOR_INTERNAL_DATA;

typedef volatile struct _THREAD_QUEUE_ENTRY THREAD_QUEUE_ENTRY, *PTHREAD_QUEUE_ENTRY;

typedef enum _PROCESSOR_STATE {
    // doing normal work such as scheduling threads
    PROCESSOR_STATE_NORMAL,
    // we are currently in an interrupt handler
    PROCESSOR_STATE_INTERRUPT
} PROCESSOR_STATE;

typedef enum _PROCESSOR_CONTROL {
    PROCESSOR_CONTROL_EXECUTE // Execute spinlock
} PROCESSOR_CONTROL;

#define AcquireProcessorControl(_Processor, _Bit) {while(_interlockedbittestandset64(&_Processor->ControlLock, _Bit)) _mm_pause();}
#define ReleaseProcessorControl(_Processor, _Bit) {_interlockedbittestandreset64(&_Processor->ControlLock, _Bit);}

typedef struct _PROCESSOR {
    PROCESSOR_IDENTIFICATION_DATA Id;
    BOOLEAN ProcessorEnabled;
    UINT64 State;
    UINT64 ControlLock;
    INTERRUPT_ARRAY* Interrupts;
    INTERRUPT_SERVICE_HANDLER SystemInterruptHandlers[36];

    NOS_GDT Gdt;
    TASK_STATE_SEGMENT Tss;
    IDT_ENTRY Idt[255];

    PTHREAD_QUEUE_ENTRY ThreadQueue;
    PTHREAD_QUEUE_ENTRY BottomOfThreadQueue;

    PTHREAD_QUEUE_ENTRY SleepQueue;
    PTHREAD_QUEUE_ENTRY BottomOfSleepQueue;

    

    // This is a 2mb page located at the top of the system address space
    PROCESSOR_INTERNAL_DATA* InternalData;

    struct {
        volatile BOOLEAN Finished;
        BOOLEAN Waiting;
        PETHREAD Thread; // The handler would wait until the thread suspends then it will resume it
        REMOTE_EXECUTE_ROUTINE Routine;
        void* Context;
        NSTATUS ReturnCode;
    } RemoteExecute;
} PROCESSOR;

typedef struct _PROCESSOR_LINKED_LIST PROCESSOR_LINKED_LIST;
struct _PROCESSOR_LINKED_LIST {
    UINT Index;
    PROCESSOR Processors[64];
    PROCESSOR_LINKED_LIST* Next;
};


typedef struct {
    UINT64 NumProcessors;
    UINT   MultiprocessorsArchitecture; // defines the architecture used to control the multiprocessor system
    UINT    Architecture;
    PROCESSOR_LINKED_LIST ProcessorListHead;
    PROCESSOR_LINKED_LIST* LastProcessorList;
} KERNEL_PROCESSOR_TABLE;
KERNEL_PROCESSOR_TABLE ProcessorTable;

typedef enum _CPU_INTERRUPT_TYPE {
    CallGate = 12,
    InterruptGate = 14,
    TrapGate = 15
} CPU_INTERRUPT_TYPE;

typedef enum _NOS_SERVICE_TYPE {
    NosInternalInterruptService,
    NosIrqService,
    NosSystemInterruptService
} NOS_SERVICE_TYPE;


// Interrupts automaticly switch the page table to kernel mode

// Processor Interrupt Utilities
NSTATUS KRNLAPI ExInstallInterruptHandler(
    IN UINT32 IrqNumber,
    IN UINT Flags,
    IN INTERRUPT_SERVICE_HANDLER Handler,
    IN OPT void* Context
);

NSTATUS KRNLAPI ExRemoveInterruptHandler(
    IN UINT32 IrqNumber,
    IN INTERRUPT_SERVICE_HANDLER Handler
);

RFPROCESSOR KeGetProcessorByIndex(UINT64 Index);


void CpuReadBrandName(char* Name);

void KiInitBootCpu();
void KiDumpProcessors();

PROCESSOR* BootProcessor;