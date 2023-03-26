#include <nos/processor/processor.h>
#include <intrin.h>

void CpuReadBrandName(char* Name) {
    CPUID_DATA CpuDump;
    __cpuid((int*)&CpuDump, 0x80000002);
    unsigned int* n = (unsigned int*)Name;
    n[0] = CpuDump.eax;
    n[1] = CpuDump.ebx;
    n[2] = CpuDump.ecx;
    n[3] = CpuDump.edx;
    n[4] = 0;

}