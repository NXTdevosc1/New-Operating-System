#include <nos/processor/internal.h>


KERNEL_PROCESSOR_TABLE ProcessorTable = {0, MpNoMultiprocessorArch, ArchAmd64, {0}, &ProcessorTable.ProcessorListHead};




BOOLEAN NSYSAPI KeRegisterProcessor(IN char* ProcessorName, OUT OPT UINT64* ProcessorId) {
    if(!ProcessorName) return FALSE;
    PROCESSOR* Processor = &ProcessorTable.LastProcessorList->Processors[ProcessorTable.LastProcessorList->Index];
    Processor->ProcessorId = ProcessorTable.NumProcessors;
    Processor->ProcessorName = ProcessorName;
    if(ProcessorTable.LastProcessorList->Index == 64) {
        
        // TODO : Allocate Non Pageable Page
        while(1);
    }
    ProcessorTable.NumProcessors++;
    ProcessorTable.LastProcessorList->Index++;
    if(ProcessorId) *ProcessorId = Processor->ProcessorId;
    return TRUE;
}

PVOID KRNLAPI KeQueryProcessorById(UINT64 ProcessorId) {
    UINT64 Index = ProcessorId & 0x3F;
    UINT64 ListIndex = ProcessorId >> 6;
    PROCESSOR_LINKED_LIST* list = &ProcessorTable.ProcessorListHead;
    while(ListIndex) {
        if(!list->Next) return NULL;
        list = list->Next;
        ListIndex--;
    }
    if(list->Index - 1 < Index) return NULL;
    return &list->Processors[Index];
}