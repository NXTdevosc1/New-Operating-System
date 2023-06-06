#include <nos/nos.h>
#include <nos/processor/processor.h>
#include <nos/processor/amd64def.h>


KERNEL_PROCESSOR_TABLE ProcessorTable = {0, MpNoMultiprocessorArch, ArchAmd64, {0}, &ProcessorTable.ProcessorListHead};

PROCESSOR_INTERNAL_DATA* InternalProcessorArray = (PROCESSOR_INTERNAL_DATA*)0xFFFFFFD000000000;

UINT64 NumProcessors = 0;

NSTATUS ProcessorEvt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 DesiredAccess) {
    return STATUS_SUCCESS;
}

RFPROCESSOR KRNLAPI KeRegisterProcessor(PROCESSOR_IDENTIFICATION_DATA* Ident) {
    if(KeGetProcessorById(Ident->ProcessorId)) {
        KDebugPrint("KeRegisterProcessor: ERR0");
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
    Processor = Object->Address;

    memcpy(&Processor->Id, Ident, sizeof(PROCESSOR_IDENTIFICATION_DATA));

    void* p;

    if(NERROR(MmAllocatePhysicalMemory(ALLOCATE_2MB_ALIGNED_MEMORY, 1, &p))) {
        KDebugPrint("KeRegisterProcessor: ERR2");
        while(1);
    }
    Processor->InternalData = InternalProcessorArray + Processor->Id.ProcessorId;
    HwMapVirtualMemory(GetCurrentPageTable(), p, Processor->InternalData, 1, PAGE_2MB | PAGE_WRITE_ACCESS, PAGE_CACHE_WRITE_BACK);
    ZeroMemory(Processor->InternalData, 0x200000);

    Processor->InternalData->Processor = Processor;
    

    _InterlockedIncrement64(&NumProcessors);
    return Processor;
}

RFPROCESSOR KRNLAPI KeGetCurrentProcessor() {
    return (InternalProcessorArray + CurrentApicId())->Processor;
}

UINT64 KRNLAPI KeGetCurrentProcessorId() {
    return KeGetCurrentProcessor()->Id.ProcessorId;
}

RFPROCESSOR KRNLAPI KeGetProcessorById(UINT64 ProcessorId) {
    if(ProcessorId >= NumProcessors) return NULL;
    return (InternalProcessorArray + ProcessorId)->Processor;
}