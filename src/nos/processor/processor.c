#include <processor/processor.h>
typedef struct _PROCESSOR_LINKED_LIST PROCESSOR_LINKED_LIST;
typedef struct _PROCESSOR_LINKED_LIST {
    UINT Index;
    PROCESSOR Processors[64];
    PROCESSOR_LINKED_LIST* Next;
};

struct {
    UINT64 NumProcessors;
    UINT   MultiprocessorsArchitecture; // defines the architecture used to control the multiprocessor system
    PROCESSOR_LINKED_LIST ProcessorListHead;
    PROCESSOR_LINKED_LIST* LastProcessorList;
} ProcessorTable = {0};


BOOLEAN NOSAPI KeRegisterProcessor(IN UINT16* ProcessorName, OUT OPT UINT64* ProcessorId) {
    if(!ProcessorName) return FALSE;
    if(!ProcessorTable.LastProcessorList) {
        // Initialize the processor table
        ProcessorTable.LastProcessorList = &ProcessorTable.ProcessorListHead;
    }
    PROCESSOR* Processor = &ProcessorTable.LastProcessorList->Processors[ProcessorTable.LastProcessorList->Index];
    Processor->ProcessorId = ProcessorTable.NumProcessors;
    Processor->ProcessorName = ProcessorName;
    if(ProcessorTable.LastProcessorList->Index == 64) {
        // TODO : Allocate Non Pageable Page
        while(1);
    }
    ProcessorTable.NumProcessors++;
    return TRUE;
}