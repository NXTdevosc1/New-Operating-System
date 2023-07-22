#include <nos/nos.h>
#include <nos/processor/processor.h>
#include <intmgr.h>

struct {
    UINT InterruptRouter; // 0 = PIC , 1 = IOAPIC
    IR_SET_INTERRUPT SetInterrupt;
    IR_REMOVE_INTERRUPT RemoveInterrupt;
    IR_TERMINATE_ROUTER TerminateRouter;
    IR_GET_INTERRUPT_INFORMATION GetInterruptInformation;
    IR_END_OF_INTERRUPT Eoi;
} gInterruptRoutingTable = {0};

static inline UINT8 KiAllocateIV(PROCESSOR* Processor, UINT Irq, IM_INTERRUPT_INFORMATION* Int, INTERRUPT_SERVICE_HANDLER Handler, void* Context) {
    INTERRUPT_DESCRIPTOR* en;
    KDebugPrint("A");
    if(Irq != -1) {
    KDebugPrint("C %x %x", Processor, Processor->Interrupts);

        // Find entry with same IRQ
        for(int i = 0;i<188;i++) {
            if(Processor->Interrupts->Interrupts[i].Present &&
            Processor->Interrupts->Interrupts[i].Irq == Irq
            ) {
                KDebugPrint("B");
                UINT64 cpf = ExAcquireSpinLock(&Processor->Interrupts->Interrupts[i].SpinLock);
                ULONG Index;
                if(!_BitScanForward64(&Index, ~Processor->Interrupts->Interrupts[i].Present)) {
                    KeRaiseException(STATUS_OUT_OF_INTERRUPTS);
                };
                KDebugPrint("B");

                en = Processor->Interrupts->Interrupts[i].Descriptors + Index;
                en->Handler = Handler;
                en->Context = Context;
                if(Processor->Interrupts->Interrupts[i].Present == 0) {
                KDebugPrint("Bb");

                    // Set an interrupt entry
                    CpuSetInterrupt(Processor, i + 0x20, InterruptGate, NosIrqService);
                    if(NERROR(gInterruptRoutingTable.SetInterrupt(Int->Fields.GlobalSystemInterrupt,
                    0, i + 0x20, Processor->Id.ProcessorId, Int->Fields.DeliveryMode,Int->Fields.Polarity, Int->Fields.TriggerMode
                    ))) {
                        KDebugPrint("ROUTER FAILED TO SET INTERRUPT");
                        while(1) __halt();
                    }
                KDebugPrint("Bb");

                }
                _bittestandset64(&Processor->Interrupts->Interrupts[i].Present, Index);
                ExReleaseSpinLock(&Processor->Interrupts->Interrupts[i].SpinLock, cpf);
                KDebugPrint("B");

                return i + 0x20;
            }
        }
    KDebugPrint("C");

    }
    KDebugPrint("A");

    // Otherwise allocate a new entry
    for(int i = 0;i<188;i++) {
        if(!Processor->Interrupts->Interrupts[i].Present) {
            if(_interlockedbittestandset64(&Processor->Interrupts->Interrupts[i].SpinLock, 0)) continue;
            if(Processor->Interrupts->Interrupts[i].Present) {
                _bittestandreset64(&Processor->Interrupts->Interrupts[i].SpinLock, 0);
                continue;
            }
    KDebugPrint("A");

            en = Processor->Interrupts->Interrupts[i].Descriptors;
            en->Handler = Handler;
            en->Context = Context;

            Processor->Interrupts->Interrupts[i].Present = 1;
            CpuSetInterrupt(Processor, i + 0x20, InterruptGate, NosIrqService);
            if(Irq != -1) {
                if(NERROR(gInterruptRoutingTable.SetInterrupt(Int->Fields.GlobalSystemInterrupt,
                    0, i + 0x20, Processor->Id.ProcessorId, Int->Fields.DeliveryMode,Int->Fields.Polarity, Int->Fields.TriggerMode
                    ))) {
                        KDebugPrint("ROUTER FAILED TO SET INTERRUPT BUG2");
                        while(1) __halt();
                    }
            }
            _bittestandreset64(&Processor->Interrupts->Interrupts[i].SpinLock, 0);
            return i + 0x20;
        }
    }

    KDebugPrint("BBBBUG");

    KeRaiseException(STATUS_OUT_OF_INTERRUPTS);
}

static SPINLOCK SelectCpuLock = 0;
UINT64 ProcessorSelector = 0;

// Processor Interrupt Utilities
NSTATUS KRNLAPI KeInstallInterruptHandler(
    IN OUT UINT32* InterruptVector, // -1 if no irq is assigned, returns an arbitrary irq number
    OUT UINT64* _ProcessorId,
    IN UINT Flags,
    IN INTERRUPT_SERVICE_HANDLER Handler,
    IN OPT void* Context
) {
    // Choosing a CPU
    UINT64 cpf = ExAcquireSpinLock(&SelectCpuLock);
    if(ProcessorSelector == NumProcessors) ProcessorSelector = 0;
    PROCESSOR* Processor = KeGetProcessorByIndex(ProcessorSelector);
    if(!Processor) {
        KDebugPrint("KE_INSTALL_INT BUG0");
        while(1) __halt();
    }
    ProcessorSelector++;
    KDebugPrint("CPU#%u Selected for Interrupt %u", Processor->Id.ProcessorId, (*InterruptVector));
    ExReleaseSpinLock(&SelectCpuLock, cpf);

    // If it is a device Interrupt, try to find an override for it
    INTERRUPT_ARRAY* Ints = Processor->Interrupts;
    
    UINT32 Irq = *InterruptVector;
    IM_INTERRUPT_INFORMATION InterruptInformation;
    if(Irq != -1) {
        // Get Interrupt Information
        if(!(Flags & IM_NO_OVERRIDE) && gInterruptRoutingTable.GetInterruptInformation(Irq, &InterruptInformation)) {
            Irq = InterruptInformation.Fields.GlobalSystemInterrupt;
        } else {
            KDebugPrint("WARNING Failed to query interrupt information, using Flags to determine interrupt parameters");
            if(Flags & IM_DELIVERY_EXTINT) {
            InterruptInformation.Fields.DeliveryMode = 0b111;
            } else InterruptInformation.Fields.DeliveryMode = (Flags & 0b110);

            InterruptInformation.Fields.Polarity = Flags >> 3;
            InterruptInformation.Fields.TriggerMode = Flags >> 4;
        }

        KDebugPrint("DELIVERY_MODE %d POLARITY %d TRIGGERMODE %d", InterruptInformation.Fields.DeliveryMode, InterruptInformation.Fields.Polarity, InterruptInformation.Fields.TriggerMode);
    } else KDebugPrint("Allocating arbitrary irq");

    UINT8 IntVec = KiAllocateIV(Processor, Irq, &InterruptInformation, Handler, Context);
    *InterruptVector = IntVec;
    *_ProcessorId = Processor->Id.ProcessorId;
    return STATUS_SUCCESS;
}

NSTATUS KRNLAPI KeRemoveInterruptHandler(
    IN PROCESSOR* Processor,
    IN UINT8 InterruptVector,
    IN INTERRUPT_SERVICE_HANDLER Handler
) {
    // TODO : Check if handler exists (PAGE_EXECUTABLE Check)

    /*
    TODO : Choose the processor based on the IRQ Number
    */
    INTERRUPT_ARRAY* Ints = Processor->Interrupts;

    // TODO : Acquire SpinLock
    UINT64 rflags = ExAcquireSpinLock(&Ints->Interrupts[InterruptVector].SpinLock);


    UINT32 IntIndx;
    UINT64 Bitmask = Ints->Interrupts[InterruptVector].Present;
    while(_BitScanForward64(&IntIndx, Bitmask)) {
        _bittestandreset64(&Bitmask, IntIndx);
        INTERRUPT_DESCRIPTOR* Descriptor = &Ints->Interrupts[InterruptVector].Descriptors[IntIndx];
        if(Descriptor->Handler == Handler) {
            _bittestandreset64(&Ints->Interrupts[InterruptVector].Present, IntIndx);
            ObjZeroMemory(Descriptor);

            if(!Ints->Interrupts[InterruptVector].Present) {
                CpuRemoveInterrupt(Processor, InterruptVector + 0x20);
                ExReleaseSpinLock(&Ints->Interrupts[InterruptVector].SpinLock, rflags);

            }
            return STATUS_SUCCESS;
        }
    }
    ExReleaseSpinLock(&Ints->Interrupts[InterruptVector].SpinLock, rflags);
    return STATUS_NOT_FOUND;
}


extern void* KiInternalInterrupts[];
extern void* KiIrqInterrupts[];
extern void* KiSystemInterrupts[];

// returns pointer to the stack (depends of the presence of the error code)
extern void* NosInternalInterruptHandler(UINT64 InterruptNumber, void* InterruptStack);
extern void NosIrqHandler(UINT64 InterruptNumber, void* InterruptStack);
extern void NosSystemInterruptHandler(UINT64 InterruptNumber, void* InterruptStack);


BOOLEAN KRNLAPI KeRegisterSystemInterrupt(
    UINT64 ProcessorId,
    UINT8* Vector, // Only vectors from 220-255 are accepted
    BOOLEAN UseWrapper,
    BOOLEAN DisableInterrupts, // if yes then use intgate otherwise use trapgate
    INTERRUPT_SERVICE_HANDLER Handler
) {
    PROCESSOR* Processor = KeGetProcessorById(ProcessorId);
    if(!Processor) {
        KDebugPrint("KRSI : 1");
        return FALSE;
    }
    ProcessorAcquireLock(Processor, PROCESSOR_IDT_LOCK);

    // Search for a free system interrupt entry (INTS 220-255)
    UINT Vec;
    for(Vec = 220;Vec<0x100;Vec++) {
        if(!Processor->Idt[Vec].Present) break;
    }
    if(Vec == 0x100) {
        ProcessorReleaseLock(Processor, PROCESSOR_IDT_LOCK);
        KDebugPrint("KRSI : 2");
        return FALSE; // All system interrupt slots are full
    }

    if(UseWrapper) {
        Processor->SystemInterruptHandlers[Vec - 220] = Handler;
        CpuSetInterrupt(Processor, Vec, (DisableInterrupts == TRUE) ? (InterruptGate) : (TrapGate), NosSystemInterruptService);
    } else {
        // Manually set the interrupt
        IDT_ENTRY* Entry = &Processor->Idt[Vec];
        Entry->CodeSegment = 0x08;
        Entry->Ist = 2;
        Entry->Address0 = (UINT64)Handler;
        Entry->Address1 = (UINT64)Handler >> 16;
        Entry->Address2 = (UINT64)Handler >> 32;
        Entry->Type = (DisableInterrupts == TRUE) ? (InterruptGate) : (TrapGate);
        Entry->Present = 1;
    }

    ProcessorReleaseLock(Processor, PROCESSOR_IDT_LOCK);
    KDebugPrint("Registered System INT#%d for CPU#%d", Vec, Processor->Id.ProcessorId);
    *Vector = Vec;
    return TRUE;
}

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



NSTATUS KRNLAPI KiSetInterruptRouter(
    UINT Router, // 00 = PIC 01 = IOAPIC
    IR_SET_INTERRUPT SetInterrupt,
    IR_REMOVE_INTERRUPT RemoveInterrupt,
    IR_TERMINATE_ROUTER TerminateRouter,
    IR_GET_INTERRUPT_INFORMATION GetInterruptInformation,
    IR_END_OF_INTERRUPT Eoi
) {
    if(gInterruptRoutingTable.InterruptRouter) {
        gInterruptRoutingTable.TerminateRouter();
    }
    gInterruptRoutingTable.InterruptRouter = Router;
    gInterruptRoutingTable.SetInterrupt = SetInterrupt;
    gInterruptRoutingTable.RemoveInterrupt = RemoveInterrupt;
    gInterruptRoutingTable.TerminateRouter = TerminateRouter;
    gInterruptRoutingTable.GetInterruptInformation = GetInterruptInformation;
    gInterruptRoutingTable.Eoi = Eoi;
    return STATUS_SUCCESS;
}

UINT64 NumIrqs = 0;

void __fastcall NosIrqHandler(UINT64 InterruptNumber, void* InterruptStack) {
    KDebugPrint("IRQ %d", InterruptNumber);

    INTERRUPT_HANDLER_DATA HandlerData = {0};
    UINT64 p = BootProcessor->Interrupts->Interrupts[InterruptNumber].Present;
    UINT Index;
    while(_BitScanForward64(&Index, p)) {
        _bittestandreset64(&p, Index);
        INTERRUPT_DESCRIPTOR* Desc = BootProcessor->Interrupts->Interrupts[InterruptNumber].Descriptors + Index;
        HandlerData.Context = Desc->Context;
        NSTATUS Status = Desc->Handler(&HandlerData);
    }

    // memset(NosInitData->FrameBuffer.BaseAddress, (NumIrqs & 1) ? 0xFF : 0, 0x5000 * 4);
    NumIrqs++;
    gInterruptRoutingTable.Eoi();
    KDebugPrint("EOI");
}

BOOLEAN KRNLAPI KeQueryInterruptInformation(UINT Irq, PIM_INTERRUPT_INFORMATION InterruptInformation) {
    return gInterruptRoutingTable.GetInterruptInformation(Irq, InterruptInformation);
}