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
void KiCpuInitDescriptors(PROCESSOR* Processor) {
    _disable();
    // Creating the interrupt array table
    INTERRUPT_ARRAY* Interrupts;
    if(NERROR(KeAllocatePhysicalMemory(0, ConvertToPages(sizeof(INTERRUPT_ARRAY)), &Interrupts))) {
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

    // Allocate CPU Internal Interrupts stack memory (INT 0-31)
    if(NERROR(KeAllocatePhysicalMemory(0, 10, (void**)&Processor->Tss.rsp0))) {
        SerialLog("Failed to allocate interrupt memory.");
        while(1);
    }
    // Allocate IRQ stack memory (INT 32-220)
    if(NERROR(KeAllocatePhysicalMemory(0, 10, (void**)&Processor->Tss.ist1))) {
        SerialLog("Failed to allocate interrupt memory.");
        while(1);
    }
    // Allocate system interrupt stack memory (INT 221-255)
    if(NERROR(KeAllocatePhysicalMemory(0, 10, (void**)&Processor->Tss.ist2))) {
        SerialLog("Failed to allocate interrupt memory.");
        while(1);
    }

    Processor->Tss.IOPB_offset = sizeof(TASK_STATE_SEGMENT);

    // Load the GDT and the TSS
    SYSTEM_DESCRIPTOR Gdtr = (SYSTEM_DESCRIPTOR){sizeof(NOS_GDT) - 1, &Processor->Gdt};

    __SystemLoadGDTAndTSS(&Gdtr);
    SerialLog("GDT and TSS Loaded successfully");

    // Initialize Standard Interrupts
    for(int i = 0;i<255;i++) {

        KiSetInterrupt(
            Processor,
            i,
            TrapGate,
            NosInternalInterruptService
        );
    }
}

extern void* KiInternalInterrupts[];
extern void* KiIrqInterrupts[];
extern void* KiSystemInterrupts[];


void NosInternalInterruptHandler(UINT64 InterruptNumber) {
    SerialLog("Internal Interrupt!");
    char bf[100];
    _ui64toa(InterruptNumber, bf, 10);
    SerialLog(bf);
    while(1);
}

void NosIrqHandler(UINT64 InterruptNumber) {

}

void NosSystemInterruptHandler(UINT64 InterruptNumber) {
    
}

void KiSetInterrupt(
    PROCESSOR* Processor,
    UINT8 InterruptNumber,
    UINT8 InterruptType,
    UINT8 NosServiceType
) {
    IDT_ENTRY* Entry = &Processor->Idt[InterruptNumber];
    
    /*
    - Handler is determined by nos service type
    */
    Entry->CodeSegment = 0x08;


    UINT64 Handler;

    if(NosServiceType == NosInternalInterruptService) {
        Entry->Ist = 0;
        Handler = (UINT64)KiInternalInterrupts[InterruptNumber];
    } else if(NosServiceType == NosIrqService) {
        Entry->Ist = 1;
        Handler = (UINT64)KiIrqInterrupts[InterruptNumber - 0x20];
    } else if(NosServiceType == NosSystemInterruptService) {
        Entry->Ist = 2;
        Handler = (UINT64)KiSystemInterrupts[InterruptNumber - 220];
    } else {
        // RAISE_HARD_ERROR
        SerialLog("KiSetInterrupt ERROR0");
        while(1);
    }

    Entry->Address0 = Handler;
    Entry->Address1 = Handler >> 16;
    Entry->Address2 = Handler >> 32;
    Entry->Type = InterruptType;
    Entry->Present = 1;
}

void KiRemoveInterrupt(
    PROCESSOR* Processor,
    UINT8 InterruptNumber
) {
    ObjZeroMemory(&Processor->Idt[InterruptNumber]);
}