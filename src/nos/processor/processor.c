#include <nos/processor/processor.h>
#include <nos/nos.h>
typedef struct _PROCESSOR_LINKED_LIST PROCESSOR_LINKED_LIST;
struct _PROCESSOR_LINKED_LIST {
    UINT Index;
    PROCESSOR Processors[64];
    PROCESSOR_LINKED_LIST* Next;
};

struct {
    UINT64 NumProcessors;
    UINT   MultiprocessorsArchitecture; // defines the architecture used to control the multiprocessor system
    UINT    Architecture;
    PROCESSOR_LINKED_LIST ProcessorListHead;
    PROCESSOR_LINKED_LIST* LastProcessorList;
} ProcessorTable = {0, MpNoMultiprocessorArch, ArchAmd64, {0}, &ProcessorTable.ProcessorListHead};

char BootProcessorName[MAX_PROCESSOR_NAME_LENGTH] = {0};

void KiInitBootCpu() {
    SerialLog("KernelInternals : Init Boot CPU");
    CpuReadBrandName(BootProcessorName);
    SerialLog("KernelInternals : Register Processor");

    KeRegisterProcessor(BootProcessorName, NULL);
}

void KiDumpProcessors() {
    SerialLog("KiDumpProcessors : (NumCpus, Arch, MpArch)");
    PROCESSOR_LINKED_LIST* l = &ProcessorTable.ProcessorListHead;
    do {
        for(UINT i = 0;i<l->Index;i++) {
            SerialLog("Processor (Pid, Name, Arch)");
            SerialLog(l->Processors[i].ProcessorName);
        }
        
        l = l->Next;
    } while(l);
}
BOOLEAN NOSAPI KeRegisterProcessor(IN char* ProcessorName, OUT OPT UINT64* ProcessorId) {
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
