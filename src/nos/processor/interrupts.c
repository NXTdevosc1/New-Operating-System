#include <nos/processor/internal.h>

// Processor Interrupt Utilities
NSTATUS KRNLAPI KeInstallInterruptHandler(
    IN UINT32 IrqNumber,
    IN INTERRUPT_SERVICE_HANDLER Handler,
    IN OPT void* Context
) {
    // TODO : Check if handler exists (PAGE_EXECUTABLE Check)

    /*
    TODO : Choose the processor based on the IRQ Number
    */
    PROCESSOR* Processor = BootProcessor;

    INTERRUPT_ARRAY* Ints = Processor->Interrupts;

    // TODO : Acquire SpinLock
    UINT64 rflags = KeAcquireSpinLock(&Ints->Interrupts[IrqNumber].SpinLock);
    UINT32 IntIndx;
    if(!Ints->Interrupts[IrqNumber].Present) {
        KiSetInterrupt(
            Processor,
            IrqNumber + 0x20,
            InterruptGate,
            NosIrqService
        );
    }
    if(!_BitScanForward64(&IntIndx, ~Ints->Interrupts[IrqNumber].Present)) {
        KeReleaseSpinLock(&Ints->Interrupts[IrqNumber].SpinLock, rflags);
        return STATUS_NO_FREE_SLOTS;
    }
    _bittestandset64(&Ints->Interrupts[IrqNumber].Present, IntIndx);
    INTERRUPT_DESCRIPTOR* Descriptor = &Ints->Interrupts[IrqNumber].Descriptors[IntIndx];
    Descriptor->Context = Context;
    Descriptor->Handler = Handler;
    KeReleaseSpinLock(&Ints->Interrupts[IrqNumber].SpinLock, rflags);

    return STATUS_SUCCESS;
}

NSTATUS KRNLAPI KeRemoveInterruptHandler(
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
    UINT64 rflags = KeAcquireSpinLock(&Ints->Interrupts[IrqNumber].SpinLock);


    UINT32 IntIndx;
    UINT64 Bitmask = Ints->Interrupts[IrqNumber].Present;
    while(_BitScanForward64(&IntIndx, Bitmask)) {
        _bittestandreset64(&Bitmask, IntIndx);
        INTERRUPT_DESCRIPTOR* Descriptor = &Ints->Interrupts[IrqNumber].Descriptors[IntIndx];
        if(Descriptor->Handler == Handler) {
            _bittestandreset64(&Ints->Interrupts[IrqNumber].Present, IntIndx);
            ObjZeroMemory(Descriptor);

            if(!Ints->Interrupts[IrqNumber].Present) {
                KiRemoveInterrupt(Processor, IrqNumber + 0x20);
                KeReleaseSpinLock(&Ints->Interrupts[IrqNumber].SpinLock, rflags);

            }
            return STATUS_SUCCESS;
        }
    }
    KeReleaseSpinLock(&Ints->Interrupts[IrqNumber].SpinLock, rflags);
    return STATUS_NOT_FOUND;
}