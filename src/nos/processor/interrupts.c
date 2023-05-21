#include <nos/processor/internal.h>

// Processor Interrupt Utilities
NSTATUS KRNLAPI ExInstallInterruptHandler(
    IN UINT32 IrqNumber,
    IN INTERRUPT_SERVICE_HANDLER Handler,
    IN OPT void* Context
) {
    /*
    TODO : Choose the processor based on the IRQ Number
    */
    PROCESSOR* Processor = BootProcessor;

    INTERRUPT_ARRAY* Ints = Processor->Interrupts;

    // TODO : Acquire SpinLock
    UINT64 rflags = ExAcquireSpinLock(&Ints->Interrupts[IrqNumber].SpinLock);
    UINT32 IntIndx;
    if(!Ints->Interrupts[IrqNumber].Present) {
        CpuSetInterrupt(
            Processor,
            IrqNumber + 0x20,
            InterruptGate,
            NosIrqService
        );
    }
    if(!_BitScanForward64(&IntIndx, ~Ints->Interrupts[IrqNumber].Present)) {
        ExReleaseSpinLock(&Ints->Interrupts[IrqNumber].SpinLock, rflags);
        return STATUS_NO_FREE_SLOTS;
    }
    _bittestandset64(&Ints->Interrupts[IrqNumber].Present, IntIndx);
    INTERRUPT_DESCRIPTOR* Descriptor = &Ints->Interrupts[IrqNumber].Descriptors[IntIndx];
    Descriptor->Context = Context;
    Descriptor->Handler = Handler;
    ExReleaseSpinLock(&Ints->Interrupts[IrqNumber].SpinLock, rflags);

    return STATUS_SUCCESS;
}

NSTATUS KRNLAPI ExRemoveInterruptHandler(
    IN UINT32 IrqNumber,
    IN INTERRUPT_SERVICE_HANDLER Handler
) {
    // TODO : Check if handler exists (PAGE_EXECUTABLE Check)

    /*
    TODO : Choose the processor based on the IRQ Number
    */
    PROCESSOR* Processor = BootProcessor;
    INTERRUPT_ARRAY* Ints = Processor->Interrupts;

    // TODO : Acquire SpinLock
    UINT64 rflags = ExAcquireSpinLock(&Ints->Interrupts[IrqNumber].SpinLock);


    UINT32 IntIndx;
    UINT64 Bitmask = Ints->Interrupts[IrqNumber].Present;
    while(_BitScanForward64(&IntIndx, Bitmask)) {
        _bittestandreset64(&Bitmask, IntIndx);
        INTERRUPT_DESCRIPTOR* Descriptor = &Ints->Interrupts[IrqNumber].Descriptors[IntIndx];
        if(Descriptor->Handler == Handler) {
            _bittestandreset64(&Ints->Interrupts[IrqNumber].Present, IntIndx);
            ObjZeroMemory(Descriptor);

            if(!Ints->Interrupts[IrqNumber].Present) {
                CpuRemoveInterrupt(Processor, IrqNumber + 0x20);
                ExReleaseSpinLock(&Ints->Interrupts[IrqNumber].SpinLock, rflags);

            }
            return STATUS_SUCCESS;
        }
    }
    ExReleaseSpinLock(&Ints->Interrupts[IrqNumber].SpinLock, rflags);
    return STATUS_NOT_FOUND;
}


extern void* KiInternalInterrupts[];
extern void* KiIrqInterrupts[];
extern void* KiSystemInterrupts[];

// returns pointer to the stack (depends of the presence of the error code)
extern void* NosInternalInterruptHandler(UINT64 InterruptNumber, void* InterruptStack);
extern void NosIrqHandler(UINT64 InterruptNumber, void* InterruptStack);
extern void NosSystemInterruptHandler(UINT64 InterruptNumber, void* InterruptStack);

void CpuSetInterrupt(
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

void CpuRemoveInterrupt(
    PROCESSOR* Processor,
    UINT8 InterruptNumber
) {
    ObjZeroMemory(&Processor->Idt[InterruptNumber]);
}