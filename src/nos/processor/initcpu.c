#include <nos/nos.h>
#include <nos/processor/processor.h>

char BootProcessorName[MAX_PROCESSOR_NAME_LENGTH];

void CpuInitDescriptors(void* Processor);

PROCESSOR* BootProcessor;

void CpuInitApicTimer();

void CpuEnableFeatures();

void KiInitBootCpu() {
    CpuEnableFeatures();
    SerialLog("KernelInternals : Init Boot CPU");
    CpuReadBrandName(BootProcessorName);
    SerialLog("KernelInternals : Register Processor");
    UINT64 ProcessorId;
    KeRegisterProcessor(BootProcessorName, &ProcessorId);
    void* Processor = KeQueryProcessorById(ProcessorId);
    CpuInitDescriptors(Processor);
    BootProcessor = Processor;
    
    

}

#define PAT_MSR 0x277

// PAT Memory Types
#define PAT_UNCACHEABLE 0
#define PAT_WRITE_COMBINING 1
#define PAT_WRITE_THROUGH 4
#define PAT_WRITE_PROTECT 5
#define PAT_WRITE_BACK 6
#define PAT_UNCACHED 7

union {
    struct {
        UINT8 Values[8];
    } Fields;
    UINT64 PageAttrTable;
} _Amd64PageAttributeTable = {
        PAT_WRITE_BACK,
        PAT_WRITE_COMBINING,
        PAT_UNCACHEABLE,
        PAT_WRITE_PROTECT,
        PAT_WRITE_THROUGH,
        PAT_WRITE_BACK,
        PAT_WRITE_BACK,
        PAT_WRITE_BACK
};

void CpuEnableFeatures() {
// Enable SSE and FPU Exceptions
CPUID_DATA Cpuid;
    // Clear EM and Set Monitor CoProcessor
    __writecr0((__readcr0() & ~(1 << 2)) | 2);
    // Enable OSFXCSR, OSXMMEXCPT, GLOBAL Pages
    __writecr4(__readcr4() | (3 << 9) | (1 << 7));
    // NX (Required)
    __cpuid(&Cpuid, 0x80000001);
    if(!(Cpuid.edx & (1 << 20))) {
        KDebugPrint("Execute-Disable Feature is required to run the OS.");
        while(1);
    }
    __writemsr(0xC0000080, __readmsr(0xC0000080) | (1 << 11));
    // Page Attribute Table (Required)
    __writemsr(PAT_MSR, _Amd64PageAttributeTable.PageAttrTable);
}