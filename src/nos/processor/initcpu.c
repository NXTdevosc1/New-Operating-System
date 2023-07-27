#include <nos/nos.h>
#include <nos/processor/processor.h>
#include <nos/sys/sys.h>

char BootProcessorName[MAX_PROCESSOR_NAME_LENGTH];

void CpuInitDescriptors(void* Processor);

PROCESSOR* BootProcessor;

void CpuInitApicTimer();

void CpuEnableFeatures();

void KiInitBootCpu() {
    CpuEnableFeatures();
    // Enable NMI
    __outbyte(0x70, __inbyte(0x70) & 0x7F);
    __inbyte(0x71);

    // Enable Parity/Channel Check
	__outbyte(SYSTEM_CONTROL_PORT_B, __inbyte(SYSTEM_CONTROL_PORT_B) | (3 << 2));

    // Disable PIC (Legacy Interrupt Router) to prevent intervention with newer routers
    __outbyte(0xA1, 0xFF);
	__outbyte(0x21, 0xFF);

    SerialLog("KernelInternals : Init Boot CPU");
    // CpuReadBrandName(BootProcessorName);
    // SerialLog("KernelInternals : Register Processor");
    UINT64 ProcessorId;
    PROCESSOR_IDENTIFICATION_DATA Ident = {0};
    BootProcessor = KeRegisterProcessor(&Ident);

    CpuInitDescriptors(BootProcessor);
    
    

}

extern void KRNLAPI KeSchedulingSystemInit();


NSTATUS ApicSpuriousInterruptHandler(INTERRUPT_HANDLER_DATA* HandlerData) {
    KDebugPrint("***SPURIOUS_INTERRUPT***");

    // You should not send EOI for spurious interrupts
    return STATUS_SUCCESS;
}
void __declspec(noreturn) HwMultiProcessorEntry() {
    CpuEnableFeatures();
    RFPROCESSOR Processor = KeGetCurrentProcessor();
    Processor->ProcessorEnabled = TRUE;
    
    
    CpuInitDescriptors(Processor);
    
    // Enable APIC
    ApicWrite(0x80, 0); // Set TASK_PRIORITY
    ApicWrite(0xD0, 0); // Set LOGICAL_DESTINATION
    ApicWrite(0xE0, 0); // SET DESTINATION_FORMAT
    UINT8 Spurious;
    if(!KeRegisterSystemInterrupt(0, &Spurious, TRUE, TRUE, ApicSpuriousInterruptHandler)) {
        KDebugPrint("APIC Initialization failed. KeRegisterSystemInterrupt != TRUE.");
        while(1) __halt();
    }
    ApicWrite(0xF0, 0x100 | Spurious); // Set SPURIOUS_INTERRUPT_VECTOR

    KeSchedulingSystemInit();
    KDebugPrint("Processor#%d enabled.", KeGetCurrentProcessorId());

    // idling
    for(;;) __halt();
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
        while(1) __halt();
    }
    __writemsr(0xC0000080, __readmsr(0xC0000080) | (1 << 11));
    // Page Attribute Table (Required)
    __writemsr(PAT_MSR, _Amd64PageAttributeTable.PageAttrTable);
}