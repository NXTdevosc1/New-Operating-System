#include <nos/nos.h>
#include <nos/processor/processor.h>
#include <nos/processor/cpudescriptors.h>
#include <nos/processor/ints.h>
#include <nos/processor/amd64def.h>
#include <nos/processor/hw.h>

// NOS_GDT NosGdt = {
//     {0},
//     {0, 0, 0b10011010, 0, 0b1010, 0},
//     {0, 0, 0b10010010, 0, 0b1010, 0},
//     {}
// }
extern void __SystemLoadGDTAndTSS(SYSTEM_DESCRIPTOR* Gdtr);

static NSTATUS __inthalt(INTERRUPT_HANDLER_DATA* Hd) {
    // Interrupts already disabled    
    KDebugPrint("Received halt system interrupt for cpu#%d", Hd->ProcessorId);
    for(;;) __halt();
}

void CpuInitDescriptors(PROCESSOR* Processor) {
    _disable();
    // Creating the interrupt array table

    SYSTEM_DESCRIPTOR Idtr = {0xFFF, Processor->Idt};
    __lidt(&Idtr);
    // Setting up the GDT
    UINT64 TssBase = (UINT64)&Processor->Tss;
    Processor->Gdt = (NOS_GDT){
        {0},
        {0, 0, 0b10011010, 0, 0b1010, 0},
        {0, 0, 0b10010010, 0, 0b1000, 0},
        {sizeof(TASK_STATE_SEGMENT), TssBase, 0b10001001, 0, 0b1000, TssBase >> 24, TssBase >> 32, 0}
    };
    // Setting up the TSS

    // RSP0 : Allocate CPU Internal Interrupts stack memory (INT 0-31)
    if(!(Processor->Tss.rsp0 = (UINT64)MmAllocateMemory(KernelProcess, 0x10, PAGE_WRITE_ACCESS | PAGE_GLOBAL, PAGE_CACHE_WRITE_BACK))) {
        SerialLog("Failed to allocate interrupt memory.");
        while(1);
    }
    // IST1 : Allocate IRQ stack memory (INT 32-220)
    if(!(Processor->Tss.ist1 = (UINT64)MmAllocateMemory(KernelProcess, 0x10, PAGE_WRITE_ACCESS | PAGE_GLOBAL, PAGE_CACHE_WRITE_BACK))) {
        SerialLog("Failed to allocate interrupt memory.");
        while(1);
    }
    // IST2 : Allocate system interrupt stack memory (INT 221-255)
    if(!(Processor->Tss.ist2 = (UINT64)MmAllocateMemory(KernelProcess, 0x10, PAGE_WRITE_ACCESS | PAGE_GLOBAL, PAGE_CACHE_WRITE_BACK))) {
        SerialLog("Failed to allocate interrupt memory.");
        while(1);
    }

    Processor->Tss.rsp0 += 0xE000;
    Processor->Tss.ist1 += 0xE000;
    Processor->Tss.ist2 += 0xE000; // add 8 for stack alignment


    Processor->Tss.IOPB_offset = sizeof(TASK_STATE_SEGMENT);

    // Load the GDT and the TSS
    SYSTEM_DESCRIPTOR Gdtr = (SYSTEM_DESCRIPTOR){sizeof(NOS_GDT) - 1, &Processor->Gdt};

    __SystemLoadGDTAndTSS(&Gdtr);
    KDebugPrint("GDT and TSS Loaded successfully");

    // Initialize Standard Interrupts
    for(int i = 0;i<32;i++) {
        UINT t = TrapGate;
        if(i == CPU_INTERRUPT_MACHINE_CHECK_EXCEPTION ||
        i == CPU_INTERRUPT_DOUBLE_FAULT || i == CPU_INTERRUPT_DEBUG_EXCEPTION
        ) t = InterruptGate;
        CpuSetInterrupt(
            Processor,
            i,
            t,
            NosInternalInterruptService
        );
    }
    // Set shutdown/halt system interrupt 0x14 (INT 0xF0)
    Processor->SystemInterruptHandlers[0x14] = __inthalt;
    CpuSetInterrupt(Processor, SYSINT_HALT, InterruptGate, NosSystemInterruptService);

}

extern void SchedulerEntry();

void CpuEnableApicTimer() {
    PROCESSOR* Processor = KeGetCurrentProcessor();


    KDebugPrint("Enabling the apic timer, interrupt number : %d", Processor->InternalData->SchedulingTimerIv);

    // TODO : Use TSC Deadline Timer if possible

    // Setup LAPIC Timer in one shot mode ()

    ApicWrite(APIC_TIMER_DIV, 3); // Divide by 16
    ApicWrite(APIC_TIMER_LVT, APIC_LVT_INTMASK);
    // Mesure Timer frequency
    ApicWrite(APIC_TIMER_INITIAL_COUNT, -1);
    Stall(50000); // Stall for 50ms
    UINT64 Frequency = (((UINT32)-1) - ApicRead(APIC_TIMER_CURRENT_COUNT)) * 20;
    KDebugPrint("APIC Timer frequency : %d HZ", Frequency * 0x10);
    Processor->InternalData->SchedulingTimerFrequency = Frequency;
    // Set LVT and One Shot Mode
    ApicWrite(APIC_TIMER_DIV, 3); // Divide by 16
    ApicWrite(APIC_TIMER_LVT, Processor->InternalData->SchedulingTimerIv);
    ApicWrite(APIC_TIMER_INITIAL_COUNT, Frequency);
}

void CpuDisableApicTimer() {
    ApicWrite(APIC_TIMER_LVT, APIC_LVT_INTMASK);
}

void HwSendIpi(UINT8 InterruptNumber, UINT64 ProcessorId, UINT8 InterruptType, UINT8 DestinationType) {
    if(!LocalApicAddress) {
        KDebugPrint("WARNING: HwSendIpi Called while not ready");
        return;
    }

    UINT64 IcrLow = (ApicRead(APIC_ICR) & 0xfff00000) | InterruptNumber;
    if(InterruptType == IPI_SYSTEM_MANAGEMENT) {
        IcrLow |= APIC_ICR_DEST_SMI;
    } else if(InterruptType == IPI_NON_MASKABLE_INTERRUPT) {
        IcrLow |= APIC_ICR_DEST_NMI;
    }
    if(DestinationType == IPI_DESTINATION_BROADCAST_ALL) {
        IcrLow |= APIC_ICR_BROADCAST_SELF_INT;
    } else if(DestinationType == IPI_DESTINATION_BROADCAST_OTHERS) {
        IcrLow |= APIC_ICR_BROADCAST_INT;
    } else if(DestinationType == IPI_DESTINATION_SELF) {
        IcrLow |= APIC_ICR_SELF_INT;
    } else {
        ApicWrite(APIC_ICR_HIGH, (ApicRead(APIC_ICR_HIGH) & 0x00ffffff) | (ProcessorId << 24));
    }

    ApicWrite(APIC_ICR, IcrLow);

    ApicIpiWait();
}

UINT64 HwGetCurrentProcessorId() {
    return ApicRead(APIC_ID) >> 24;
}

void HwBootProcessor(RFPROCESSOR Processor) {
    KDebugPrint("Booting Processor#%d , SYSTEM: APIC", Processor->Id.ProcessorId);

    // Allocate some stack space for the cpu
    void* p;
    if(NERROR(MmAllocatePhysicalMemory(0, 0x10, &p)))
    {
        KDebugPrint("Failed to allocate mem for cpu");
        while(1) __halt();
    }
    *(UINT64*)((UINT64)NosInitData->InitTrampoline + 0x6008) = (UINT64)p + 0xE000;
    
    ApicWrite(APIC_ERROR_STATUS, 0);

    // Send INIT_IPI
    ApicWrite(APIC_ICR_HIGH, (ApicRead(APIC_ICR_HIGH) & 0x00ffffff) | (Processor->Id.ProcessorId << 24));
    ApicWrite(APIC_ICR, (ApicRead(APIC_ICR) & 0xfff00000) | APIC_ICR_DEST_INIT | APIC_ICR_SET_FOR_INIT | APIC_ICR_CLEAR_FOR_INIT);

    ApicIpiWait();
    // Send INIT_IPI Level-deassert

    ApicWrite(APIC_ICR_HIGH, (ApicRead(APIC_ICR_HIGH) & 0x00ffffff) | (Processor->Id.ProcessorId << 24));
    ApicWrite(APIC_ICR,(ApicRead(APIC_ICR) & 0xfff00000) | APIC_ICR_DEST_INIT | APIC_ICR_SET_FOR_INIT);

    ApicIpiWait();


    // Boot the AP
    for(int i = 0;i<2;i++) {
        ApicWrite(APIC_ERROR_STATUS, 0);
        ApicWrite(APIC_ICR_HIGH, (ApicRead(APIC_ICR_HIGH) & 0x00ffffff) | (Processor->Id.ProcessorId << 24));
        ApicWrite(APIC_ICR, (ApicRead(APIC_ICR) & 0xfff00000) | APIC_ICR_DEST_SIPI | ((UINT64)NosInitData->InitTrampoline >> 12));
        ApicIpiWait();
    }

    // Wait for processor boot
    // if it exceeds 3 seconds crash the OS

    for(UINT i = 0;i<6000 && !Processor->ProcessorEnabled;i++) Stall(500);
    
    if(!Processor->ProcessorEnabled) {
        KDebugPrint("Failed to boot processor#%d, halting...", Processor->Id.ProcessorId);
        while(1) __halt();
    }
}