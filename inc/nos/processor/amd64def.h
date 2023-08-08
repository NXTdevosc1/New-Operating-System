#pragma once

// APIC Definitions

#define APIC_ID 0x20
#define APIC_VER 0x30
#define APIC_TASK_PRIORITY 0x80
#define APIC_APR 0x90
#define APIC_PPR 0xA0
#define APIC_EOI 0xB0
#define APIC_RRD 0xC0
#define APIC_LDR 0xD0
#define APIC_DFR 0xE0
#define APIC_SPURIOUS 0xF0
#define APIC_INSERVICE_REGISTEER 0x100
#define APIC_TRIGGER_MODE 0x180
#define APIC_INTERRUPT_REQUEST 0x200
#define APIC_ERROR_STATUS 0x280
#define APIC_TIMER_LVT 0x320
#define APIC_TIMER_DIV 0x3E0
#define APIC_TIMER_INITIAL_COUNT 0x380
#define APIC_TIMER_CURRENT_COUNT 0x390

#define APIC_ICR 0x300
#define APIC_ICR_HIGH 0x310

#define APIC_LVT_INTMASK (1 << 16)

// INTERRUPT COMMAND REGISTER
#define APIC_ICR_DEST_NORMAL 0
#define APIC_ICR_DEST_LP (1 << 8)
#define APIC_ICR_DEST_SMI (2 << 8)
#define APIC_ICR_DEST_NMI (4 << 8)
#define APIC_ICR_DEST_INIT (5 << 8)
#define APIC_ICR_DEST_SIPI (6 << 8)

#define APIC_ICR_LOGICAL_DEST (1 << 11)
#define APIC_ICR_DELIVERY_STATUS (1 << 12)
#define APIC_ICR_CLEAR_FOR_INIT (1 << 14)
#define APIC_ICR_SET_FOR_INIT (1 << 15)

#define APIC_ICR_SELF_INT (1 << 18)
#define APIC_ICR_BROADCAST_SELF_INT (2 << 18)
#define APIC_ICR_BROADCAST_INT (3 << 18)

// MSR Definitions

// Waits upto 3 seconds
static inline void ApicIpiWait() {
    UINT __i = 0;
    _mm_pause();
    for(;__i<3000 && (ApicRead(APIC_ICR) & APIC_ICR_DELIVERY_STATUS);__i++, Stall(1000)) _mm_pause();
    if(__i == 3000) {
        KDebugPrint("IPI Timeout...");
        while(1) __halt();
    }
}