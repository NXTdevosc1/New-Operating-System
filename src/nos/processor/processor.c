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
