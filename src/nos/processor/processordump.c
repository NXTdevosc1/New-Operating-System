#include <nos/processor/processor.h>



void KiDumpProcessors() {
    SerialLog("KiDumpProcessors : (NumCpus, Arch, MpArch)");
    while(1);
    PROCESSOR_LINKED_LIST* l = &ProcessorTable.ProcessorListHead;
    do {
        for(UINT i = 0;i<l->Index;i++) {
            // KDebugPrint("")
        }
        
        l = l->Next;
    } while(l);
}