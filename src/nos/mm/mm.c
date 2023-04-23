#include <nos/nos.h>

void* KeMainThread = (void*)(UINT64)-1; // for tests

NOS_MEMORY_DESCRIPTOR* MmCreateMemoryDescriptor(
    void* PhysicalAddress,
    UINT32 Attributes,
    UINT64 NumPages
) {
    NOS_MEMORY_LINKED_LIST* mem = NosInitData->NosMemoryMap;
    unsigned long Index;
    for(;;) {
        if(_BitScanForward64(&Index, ~mem->Full)) {
            KeMutexWait(KeMainThread, &mem->GroupsMutex[Index], 0);
            unsigned long Index2;
            if(_BitScanForward64(&Index2, ~mem->Groups[Index].Present)) {
                _bittestandset64(&mem->Groups[Index].Present, Index2);
                if(mem->Groups[Index].Present == (UINT64)-1) {
                    _bittestandset64(&mem->Full, Index);
                }
                NOS_MEMORY_DESCRIPTOR* desc = &mem->Groups[Index].MemoryDescriptors[Index2];
                desc->PhysicalAddress = PhysicalAddress;
                desc->Attributes = Attributes;
                desc->NumPages = NumPages;
                KeMutexRelease(KeMainThread, &mem->GroupsMutex[Index]);
                return desc;
            }
            KeMutexRelease(KeMainThread, &mem->GroupsMutex[Index]);
        }
        if(!mem->Next) {
            SerialLog("Allocating New NOS_MEMORY_LINKED_LIST");
            if(NERROR(
            KeAllocatePhysicalMemory(
                0,
                ConvertToPages(sizeof(NOS_MEMORY_LINKED_LIST)),
                &mem->Next
            )
            )) {
                SerialLog("Failed to allocate new NOS_MEMORY_LINKED_LIST");
                // TODO : Crash
                while(1);
            }
            SerialLog("Allocation Success");
            *mem->Next = (NOS_MEMORY_LINKED_LIST){0};
            SerialLog("NEXT_MEM0");
        } else SerialLog("NEXT_MEM");
        mem = mem->Next;
    }
}