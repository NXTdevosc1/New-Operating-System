#include <nos/nos.h>
#include <nos/processor/processor.h>

char BootProcessorName[MAX_PROCESSOR_NAME_LENGTH] = {0};


void KiInitBootCpu() {
    SerialLog("KernelInternals : Init Boot CPU");
    CpuReadBrandName(BootProcessorName);
    SerialLog("KernelInternals : Register Processor");

    KeRegisterProcessor(BootProcessorName, NULL);
}