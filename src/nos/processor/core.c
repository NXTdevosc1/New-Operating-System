#include <nos/nos.h>
#include <nos/processor/processor.h>
#include <nos/processor/cpudescriptors.h>
#include <nos/processor/ints.h>
#include <nos/processor/amd64def.h>
// NOS_GDT NosGdt = {
//     {0},
//     {0, 0, 0b10011010, 0, 0b1010, 0},
//     {0, 0, 0b10010010, 0, 0b1010, 0},
//     {}
// }
extern void __SystemLoadGDTAndTSS(SYSTEM_DESCRIPTOR* Gdtr);
void CpuInitDescriptors(PROCESSOR* Processor) {
    _disable();
    // Creating the interrupt array table
    INTERRUPT_ARRAY* Interrupts;
    Interrupts = MmAllocateMemory(KernelProcess, ConvertToPages(sizeof(INTERRUPT_ARRAY)), PAGE_WRITE_ACCESS | PAGE_GLOBAL, PAGE_CACHE_WRITE_THROUGH);

    if(!Interrupts) {
        SerialLog("Failed to allocate interrupt array.");
        while(1);
    }
    ObjZeroMemory(Interrupts);
    Processor->Interrupts = Interrupts;
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

    Processor->Tss.rsp0 += 0x8000;
    Processor->Tss.ist1 += 0x8000;
    Processor->Tss.ist2 += 0x8000;


    Processor->Tss.IOPB_offset = sizeof(TASK_STATE_SEGMENT);

    // Load the GDT and the TSS
    SYSTEM_DESCRIPTOR Gdtr = (SYSTEM_DESCRIPTOR){sizeof(NOS_GDT) - 1, &Processor->Gdt};

    __SystemLoadGDTAndTSS(&Gdtr);
    SerialLog("GDT and TSS Loaded successfully");

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
}

extern void SchedulerEntry();

void CpuEnableApicTimer() {
    // Request a system interrupt
    PROCESSOR* Processor = KeGetCurrentProcessor(); // todo : get current processor


    KDebugPrint("Enabling the apic timer, interrupt number : %d", Processor->InternalData->SchedulingTimerIv);

    // TODO : Use TSC Deadline Timer if possible

    // Setup LAPIC Timer in one shot mode ()

    ApicWrite(APIC_TIMER_DIV, 3); // Divide by 16
    ApicWrite(APIC_TIMER_LVT, APIC_LVT_INTMASK);
    // Mesure Timer frequency
    ApicWrite(APIC_TIMER_INITIAL_COUNT, -1);
    Stall(5000); // Stop for 5ms
    UINT Frequency = (((UINT32)-1) - ApicRead(APIC_TIMER_CURRENT_COUNT)) * 200;
    KDebugPrint("APIC Timer frequency : %d HZ", Frequency * 0x10);

    // Set LVT and One Shot Mode
    ApicWrite(APIC_TIMER_DIV, 3); // Divide by 16
    ApicWrite(APIC_TIMER_LVT, Processor->InternalData->SchedulingTimerIv);
    ApicWrite(APIC_TIMER_INITIAL_COUNT, Frequency);
}

void CpuDisableApicTimer() {

}