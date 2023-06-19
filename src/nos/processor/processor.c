#include <nos/nos.h>
#include <nos/processor/processor.h>
#include <nos/processor/amd64def.h>

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
    // if(KeGetProcessorById(Ident->ProcessorId)) {
    //     KDebugPrint("KeRegisterProcessor: ERR0");
    //     while(1);
    // }
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
    

    _InterlockedIncrement64(&NumProcessors);
    return Processor;
}

RFPROCESSOR KRNLAPI KeGetCurrentProcessor() {
    // System still is not using the LAPIC
    if(!BootProcessor) return NULL;
    if(!BootProcessor->ProcessorEnabled) return BootProcessor;
    
    
    return ((PROCESSOR_INTERNAL_DATA*)(InternalProcessorArray + CurrentApicId()))->Processor;
}

UINT64 KRNLAPI KeGetCurrentProcessorId() {
    return KeGetCurrentProcessor()->Id.ProcessorId;
}

RFPROCESSOR KRNLAPI KeGetProcessorById(UINT64 ProcessorId) {
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