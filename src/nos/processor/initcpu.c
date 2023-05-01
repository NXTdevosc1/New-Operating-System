#include <nos/nos.h>
#include <nos/processor/processor.h>

char BootProcessorName[MAX_PROCESSOR_NAME_LENGTH] = {0};

void KiCpuInitDescriptors(void* Processor);

PROCESSOR* BootProcessor;


void KiInitBootCpu() {
    SerialLog("KernelInternals : Init Boot CPU");
    CpuReadBrandName(BootProcessorName);
    SerialLog("KernelInternals : Register Processor");
    UINT64 ProcessorId;
    KeRegisterProcessor(BootProcessorName, &ProcessorId);
    void* Processor = KeQueryProcessorById(ProcessorId);
    KiCpuInitDescriptors(Processor);
    BootProcessor = Processor;
    
    

}