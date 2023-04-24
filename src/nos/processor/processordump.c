#include <nos/nos.h>
#include <nos/processor/processor.h>




void KiDumpProcessors() {
    SerialLog("KiDumpProcessors : (NumCpus, Arch, MpArch)");
    PROCESSOR_LINKED_LIST* l = &gProcessorTable.ProcessorListHead;
    do {
        for(UINT i = 0;i<l->Index;i++) {
            SerialLog("Processor (Pid, Name, Arch)");
            SerialLog(l->Processors[i].ProcessorName);
        }
        
        l = l->Next;
    } while(l);
}