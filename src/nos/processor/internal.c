#include <nos/processor/processor.h>
#include <nos/processor/internal.h>
#include <nos/processor/cpudescriptors.h>



void KiInitDescriptorTables(PROCESSOR* Processor) {
    // Creating the interrupt array table
    INTERRUPT_ARRAY* Interrupts;
    if(NERROR(KeAllocatePhysicalMemory(0, ConvertToPages(sizeof(INTERRUPT_ARRAY)), &Interrupts))) {
        SerialLog("Failed to allocate interrupt array.");
        while(1);
    }
    ObjZeroMemory(Interrupts);
    Processor->Interrupts = Interrupts;

    // Loading the GDT
}