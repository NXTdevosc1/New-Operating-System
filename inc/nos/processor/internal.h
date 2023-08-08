#pragma once
#include <nos/nos.h>

typedef struct _PROCESSOR PROCESSOR;

typedef struct _INTERRUPT_ARRAY INTERRUPT_ARRAY;
#pragma pack(push, 1)
typedef struct _TASK_STATE_SEGMENT {
	UINT32 reserved0;
	UINT64 rsp0;
	UINT64 rsp1;
	UINT64 rsp2;
	UINT64 reserved1;
	UINT64 ist1;
	UINT64 ist2;
	UINT64 ist3;
	UINT64 ist4;
	UINT64 ist5;
	UINT64 ist6;
	UINT64 ist7;
	UINT64 reserved2;
	UINT16 reserved3;
	UINT16 IOPB_offset;
} TASK_STATE_SEGMENT;
#pragma pack(pop)

typedef struct {
    UINT64 Limit0 : 16;
    UINT64 Base0 : 24;
    UINT64 Access : 8; // P,DPL[2],SysDesc=0,Ex,DC,RW,A 
    UINT64 Limit1 : 4;
    UINT64 Flags : 4; // G,DB,L,RSV Always set to 1010
    UINT64 Base1 : 8;
} GDT_ENTRY;

typedef struct {
    UINT64 Address0 : 16;
    UINT64 CodeSegment : 16;
    UINT64 Ist : 8;
    UINT64 Type : 5;
    UINT64 PrivilegeLevel : 2;
    UINT64 Present : 1;
    UINT64 Address1 : 16;
    UINT64 Address2; // High 32 bits
} IDT_ENTRY;

typedef struct {
    UINT64 Limit0 : 16;
    UINT64 Base0 : 24;
    UINT64 Access : 8; // P,DPL[2],SysDesc=0,Ex,DC,RW,A 
    UINT64 Limit1 : 4;
    UINT64 Flags : 4; // G,DB,L,RSV Always set to 1010
    UINT64 Base1 : 8;
    UINT32 Base2;
    UINT32 Reserved;
} GDT_SYSTEM_ENTRY;


typedef struct {
    // 0x00
    GDT_ENTRY Null0;
    // 0x08
    GDT_ENTRY KernelCode;
    // 0x10
    GDT_ENTRY KernelData;
    // 0x18
    GDT_SYSTEM_ENTRY Tss;
    // 0x28
} NOS_GDT;

// PROCESSOR LOCKS

#define PROCESSOR_IDT_LOCK 0

#define ProcessorAcquireLock(_p, _bit) while(_interlockedbittestandset64(&_p->ControlLock, _bit)) _mm_pause()
#define ProcessorReleaseLock(_p, _bit) _interlockedbittestandreset64(&_p->ControlLock, _bit)



void CpuSetInterrupt(
    PROCESSOR* Processor,
    UINT8 InterruptNumber,
    UINT8 InterruptType,
    UINT8 NosServiceType
);

void CpuRemoveInterrupt(
    PROCESSOR* Processor,
    UINT8 InterruptNumber
);

