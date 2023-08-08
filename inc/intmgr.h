/*
INTERRUPT MANAGEMENT DEFINES
*/
#pragma once
#include <nosdef.h>


// Interrupt Router SET/REMOVE Flags
#define IM_NO_OVERRIDE 1
// Delivery mode
#define IM_DELIVERY_SMI 2
#define IM_DELIVERY_NMI 5
#define IM_DELIVERY_EXTINT 6
// Polarity
#define IM_ACTIVE_LOW 8
// Trigger mode
#define IM_LEVEL_TRIGGERED 0x10

// Field value are the same as IOAPIC
typedef NSTATUS(__cdecl *IR_SET_INTERRUPT)(
    UINT Irq,
    UINT Flags,
    UINT InterruptVector,
    UINT64 ProcessorId,
    UINT DeliveryMode,
    BOOLEAN Polarity,
    BOOLEAN TriggerMode
);

typedef NSTATUS(__cdecl *IR_REMOVE_INTERRUPT)(UINT Irq, UINT Flags);

// Never called
typedef NSTATUS(__cdecl *IR_TERMINATE_ROUTER)();

typedef void(__cdecl *IR_END_OF_INTERRUPT)();

typedef union _IM_INTERRUPT_INFORMATION {
    struct {
        DWORD DeliveryMode : 5;
        DWORD Polarity : 1;
        DWORD TriggerMode : 1;
        DWORD Rsv0 : 1;
        DWORD GlobalSystemInterrupt : 8;
        DWORD Bus : 8;
        DWORD Reserved : 8;
    } Fields;
    DWORD Data;
} IM_INTERRUPT_INFORMATION, *PIM_INTERRUPT_INFORMATION;

typedef struct _INTERRUPT_STACK_FRAME {
    UINT64 InstructionPointer;
    UINT64 CodeSegment;
    UINT64 Rflags;
    UINT64 StackPointer;
    UINT64 StackSegment;
} INTERRUPT_STACK_FRAME;

typedef struct _INTERRUPT_ERRCODE_STACK_FRAME {
    UINT64 ErrorCode;
    UINT64 InstructionPointer;
    UINT64 CodeSegment;
    UINT64 Rflags;
    UINT64 StackPointer;
    UINT64 StackSegment;
} INTERRUPT_ERRCODE_STACK_FRAME;


typedef struct _INTERRUPT_HANDLER_DATA {
    UINT32 InterruptNumber;
    union {
        INTERRUPT_STACK_FRAME StackFrame;
        INTERRUPT_ERRCODE_STACK_FRAME ErrcodeStackFrame;
    } StackFrame;
    UINT64 ProcessorId;
    void* Context; // Custum parameter passed by the caller to InstallIntHandler
    PETHREAD Thread;
} INTERRUPT_HANDLER_DATA;

typedef NSTATUS(*INTERRUPT_SERVICE_HANDLER)(INTERRUPT_HANDLER_DATA* InterruptHandlerData);

typedef BOOLEAN (__cdecl *IR_GET_INTERRUPT_INFORMATION)(UINT Irq, PIM_INTERRUPT_INFORMATION InterruptInformation);

typedef struct _PROCESSOR PROCESSOR;

// Interrupt Management
NSTATUS KRNLAPI KeInstallInterruptHandler(
    IN OUT UINT32* InterruptVector, // -1 if no irq is assigned, returns an arbitrary irq number
    OUT UINT64* _ProcessorId,
    IN UINT Flags,
    IN INTERRUPT_SERVICE_HANDLER Handler,
    IN OPT void* Context
);

NSTATUS KRNLAPI KeRemoveInterruptHandler(
    IN PROCESSOR* Processor,
    IN UINT8 InterruptVector,
    IN INTERRUPT_SERVICE_HANDLER Handler
);

BOOLEAN KRNLAPI KeRegisterSystemInterrupt(
    UINT64 ProcessorId,
    UINT8* Vector, // Only vectors from 220-255 are accepted
    BOOLEAN UseWrapper,
    BOOLEAN DisableInterrupts, // if yes then use intgate otherwise use trapgate
    INTERRUPT_SERVICE_HANDLER Handler
);