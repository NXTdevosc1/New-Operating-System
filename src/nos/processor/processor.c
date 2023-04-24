#include <nos/nos.h>
#include <nos/processor/processor.h>


KERNEL_PROCESSOR_TABLE gProcessorTable = {0, MpNoMultiprocessorArch, ArchAmd64, {0}, &gProcessorTable.ProcessorListHead};




BOOLEAN NSYSAPI KeRegisterProcessor(IN char* ProcessorName, OUT OPT UINT64* ProcessorId) {
    if(!ProcessorName) return FALSE;
    PROCESSOR* Processor = &gProcessorTable.LastProcessorList->Processors[gProcessorTable.LastProcessorList->Index];
    Processor->ProcessorId = gProcessorTable.NumProcessors;
    Processor->ProcessorName = ProcessorName;
    if(gProcessorTable.LastProcessorList->Index == 64) {
        
        // TODO : Allocate Non Pageable Page
        while(1);
    }
    gProcessorTable.NumProcessors++;
    gProcessorTable.LastProcessorList->Index++;
    if(ProcessorId) *ProcessorId = Processor->ProcessorId;
    return TRUE;
}
