#include <nos/nos.h>
#include <nos/processor/processor.h>
#include <nos/processor/amd64def.h>
#include <nos/processor/hw.h>

typedef struct _PAGEDIR_REPRESENTATION {
    UINT8 buffer[0x200000];
} PDRPRS;

KERNEL_PROCESSOR_TABLE ProcessorTable = {0, MpNoMultiprocessorArch, ArchAmd64, {0}, &ProcessorTable.ProcessorListHead};

PDRPRS* InternalProcessorArray = (PDRPRS*)0xFFFFFFD000000000;

volatile UINT64 NumProcessors = 0;

NSTATUS ProcessorEvt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 DesiredAccess) {
    return STATUS_SUCCESS;
}

RFPROCESSOR KRNLAPI KeRegisterProcessor(PROCESSOR_IDENTIFICATION_DATA* Ident) {
    if(KeGetProcessorById(Ident->ProcessorId)) {
        KDebugPrint("KeRegisterProcessor: ERR0 Existing Processor Id Registered %u", Ident->ProcessorId);
        while(1);
    }
    POBJECT Object;
    RFPROCESSOR Processor;
    if(NERROR(ObCreateObject(
        NULL,
        &Object,
        OBJECT_PERMANENT,
        OBJECT_PROCESSOR,
        NULL,
        sizeof(PROCESSOR),
        ProcessorEvt
    ))) {
        KDebugPrint("KeRegisterProcessor: ERR1");
        while(1);
    }
    // Manually set object id
    Object->ObjectId = Ident->ProcessorId;

    Processor = Object->Address;

    memcpy(&Processor->Id, Ident, sizeof(PROCESSOR_IDENTIFICATION_DATA));

    void* p;

    if(NERROR(MmAllocatePhysicalMemory(ALLOCATE_2MB_ALIGNED_MEMORY, 1, &p))) {
        KDebugPrint("KeRegisterProcessor: ERR2");
        while(1);
    }
    Processor->InternalData = (void*)(InternalProcessorArray + Processor->Id.ProcessorId);
    HwMapVirtualMemory(GetCurrentPageTable(), p, Processor->InternalData, 1, PAGE_2MB | PAGE_WRITE_ACCESS, PAGE_CACHE_WRITE_BACK);
    ZeroMemory(Processor->InternalData, 0x200000);

    Processor->InternalData->Processor = Processor;

    Processor->Interrupts = MmAllocateMemory(KernelProcess, ConvertToPages(sizeof(INTERRUPT_ARRAY)), PAGE_WRITE_ACCESS | PAGE_GLOBAL, PAGE_CACHE_WRITE_THROUGH);
    if(!Processor->Interrupts) {
        SerialLog("Failed to allocate interrupt array.");
        while(1);
    }
    ObjZeroMemory(Processor->Interrupts);
    

    _InterlockedIncrement64(&NumProcessors);

    return Processor;
}

RFPROCESSOR KRNLAPI KeGetCurrentProcessor() {
    // System still is not using the LAPIC
    if(!BootProcessor) {
        SerialLog("KERNEL BUG 0: get processor before initialization");
        while(1) __halt();
    }
    if(!BootProcessor->ProcessorEnabled) return BootProcessor;
    
    
    return ((PROCESSOR_INTERNAL_DATA*)(InternalProcessorArray + CurrentApicId()))->Processor;
}

UINT64 KRNLAPI KeGetCurrentProcessorId() {
    
    return KeGetCurrentProcessor()->Id.ProcessorId;
}

RFPROCESSOR KRNLAPI KeGetProcessorById(UINT64 ProcessorId) {
    if(!BootProcessor) return NULL;
    PROCESSOR_INTERNAL_DATA* data = ((PROCESSOR_INTERNAL_DATA*)(InternalProcessorArray + ProcessorId));
    UINT64 f;
    if(!KeCheckMemoryAccess(NULL, data, 0x1000, NULL)) return NULL;
    return data->Processor;
}

RFPROCESSOR KeGetProcessorByIndex(UINT64 Index) {
    UINT64 _ev = 0;
    POBJECT Obj;
    while((_ev = ObEnumerateObjects(NULL, OBJECT_PROCESSOR, &Obj, NULL, _ev)) != 0) {
        if(!Index) return Obj->Address;
        Index--;
    }
    return NULL;
}

// if wait, returns status of the remote execution routine
// otherwise it returns STATUS_SUCCESS
NSTATUS KRNLAPI KeRemoteExecute(PROCESSOR* Processor, REMOTE_EXECUTE_ROUTINE Routine, void* Context, BOOLEAN Wait) {
    
    // Processor will automatically releasse the control bit when the routine is done
    AcquireProcessorControl(Processor, PROCESSOR_CONTROL_EXECUTE);

    Processor->RemoteExecute.Routine = Routine;
    Processor->RemoteExecute.Context = Context;
    Processor->RemoteExecute.Finished = FALSE;
    Processor->RemoteExecute.Thread = KeGetCurrentThread();
    Processor->RemoteExecute.Waiting = Wait;
    HwSendIpi(SYSINT_EXECUTE, Processor->Id.ProcessorId, IPI_NORMAL, IPI_DESTINATION_NORMAL);
    
    NSTATUS Status = STATUS_SUCCESS;

    if(Wait) {
        for(;;) {
            __wbinvd(); // Update cache
            if(Processor->RemoteExecute.Finished == FALSE) _mm_pause();
            else {
                Status = Processor->RemoteExecute.ReturnCode;
                break;
            }
            // If resumed finished should read as true because of cache update
        }
        // Manually release control if its synchronous
        ReleaseProcessorControl(Processor, PROCESSOR_CONTROL_EXECUTE);
    }

    return Status;
}

void KRNLAPI KeProcessorReadIdentificationData(RFPROCESSOR Processor, PROCESSOR_IDENTIFICATION_DATA* Identification) {
    memcpy(Identification, &Processor->Id, sizeof(PROCESSOR_IDENTIFICATION_DATA));
}