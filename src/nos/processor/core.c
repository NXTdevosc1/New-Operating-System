#include <nos/processor/internal.h>
#include <nos/processor/processor.h>
#include <nos/processor/cpudescriptors.h>
#include <nos/processor/ints.h>

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
    if(NERROR(MmAllocatePhysicalMemory(0, ConvertToPages(sizeof(INTERRUPT_ARRAY)), &Interrupts))) {
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

    Processor->Tss.rsp0 += 0xF000;
    Processor->Tss.ist1 += 0xF000;
    Processor->Tss.ist2 += 0xF000;


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

void CpuInitApicTimer() {
    
}





