#include <nos/nos.h>

void NOSINTERNAL KiPhysicalMemoryManagerInit() {
    
    NOS_MEMORY_LINKED_LIST* PhysicalMem = NosInitData->NosMemoryMap;
    unsigned long Index = 0;
    UINT64 AllocatedMemory = 0, FreeMemory = 0;
    do {
        for(int c=0;c<0x40;c++) {
            if(PhysicalMem->Groups[c].Present) {
                UINT64 Mask = PhysicalMem->Groups[c].Present;
                while(_BitScanForward64(&Index, Mask)) {
                    _bittestandreset64(&Mask, Index);
                    NOS_MEMORY_DESCRIPTOR* Mem = &PhysicalMem->Groups[c].MemoryDescriptors[Index];
                    if(Mem->Attributes & MM_DESCRIPTOR_ALLOCATED) {
                        AllocatedMemory += Mem->NumPages * 0x1000;
                    }
                    else {
                        FreeMemory += Mem->NumPages * 0x1000;
                    }
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    } while(PhysicalMem);
    KDebugPrint("Kernel boot memory : Used %d bytes , free : %d bytes, total %d bytes", AllocatedMemory, FreeMemory, AllocatedMemory + FreeMemory);

    NosInitData->AllocatedPagesCount = AllocatedMemory >> 12;
    NosInitData->TotalPagesCount = (AllocatedMemory + FreeMemory) >> 12;
}