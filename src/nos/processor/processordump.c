#include <nos/processor/internal.h>



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