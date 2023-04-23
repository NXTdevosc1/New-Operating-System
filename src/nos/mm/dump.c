#include <nos/nos.h>

void NOSINTERNAL KiDumpPhysicalMemoryEntries() {
    
    NOS_MEMORY_LINKED_LIST* PhysicalMem = NosInitData->NosMemoryMap;
    unsigned long Index;
    char format[30];
    UINT64 AllocatedMemory = 0, FreeMemory = 0;
    do {
        for(int c=0;c<0x40;c++) {
            if(PhysicalMem->Groups[c].Present) {

                UINT64 Mask = PhysicalMem->Groups[c].Present;
                while(_BitScanForward64(&Index, Mask)) {
                    _bittestandreset64(&Mask, Index);
                    NOS_MEMORY_DESCRIPTOR* Mem = &PhysicalMem->Groups[c].MemoryDescriptors[Index];
                    if(Mem->Attributes & MM_DESCRIPTOR_ALLOCATED) {
                        SerialLog("Allocated");
                        AllocatedMemory += Mem->NumPages * 0x1000;
                    }
                    else {
                        SerialLog("Physical Memory Entry : (Attributes, PhysStart, NumPages)");
                        _ui64toa((UINT64)Mem->PhysicalAddress, format, 0x10);
                        SerialLog(format);
                        _ui64toa((UINT64)Mem->NumPages, format, 10);
                        SerialLog(format);
                        FreeMemory += Mem->NumPages * 0x1000;
                    }
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    } while(PhysicalMem);
    SerialLog("Used Memory, Free Memory, Total Memory (In Bytes)");
    _ui64toa(AllocatedMemory, format, 10);
    SerialLog(format);
    _ui64toa(FreeMemory, format, 10);
    SerialLog(format);
    _ui64toa(AllocatedMemory + FreeMemory, format, 10);
    SerialLog(format);
    SerialLog("_______________________________________");
}