#include <nos/mm/physmem.h>

NSTATUS KRNLAPI KeAllocatePhysicalMemory(UINT64 Flags, UINT64 NumPages, void** Ptr) {
    if(Flags & ALLOCATE_BELOW_4GB) {
        return MmAllocateLowMemory(Flags, NumPages, Ptr);
    } else {
        // Search memory above 4gb then search low memory
        NSTATUS s = MmAllocateHighMemory(Flags, NumPages, Ptr);
        if(NERROR(s)) return MmAllocateLowMemory(Flags, NumPages, Ptr);
        return s;
    }
}

// Allocates memory below 4gb
NSTATUS KRNLAPI MmAllocateLowMemory(
    UINT64 Flags, UINT64 NumPages, void** Ptr
) {
    NOS_MEMORY_LINKED_LIST* PhysicalMem = NosInitData->NosMemoryMap;
    unsigned long Index;
    UINT64 NumBytes = NumPages << 12;
    UINT64 Align = 0x1000;
    while(PhysicalMem) {
        for(int c = 0;c<0x40;c++) {
            if(PhysicalMem->Groups[c].Present) {
                UINT64 Mask = PhysicalMem->Groups[c].Present;
                while(_BitScanForward64(&Index, Mask)) {
                    _bittestandreset64(&Mask, Index);
                    NOS_MEMORY_DESCRIPTOR* Mem = &PhysicalMem->Groups[c].MemoryDescriptors[Index];
                    if(((UINT64)Mem->PhysicalAddress + ) > 0xFFFFF000)
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    }
}



// Allocates memory above 4gb
NSTATUS KRNLAPI MmAllocateHighMemory(
    UINT64 Flags, UINT64 NumPages, void** Ptr
) {
    NOS_MEMORY_LINKED_LIST* PhysicalMem = NosInitData->NosMemoryMap;
    unsigned long Index;
    while(PhysicalMem) {
        for(int c = 0;c<0x40;c++) {
            if(PhysicalMem->Groups[c].Present) {
                UINT64 Mask = PhysicalMem->Groups[c].Present;
                while(_BitScanForward64(&Index, Mask)) {
                    _bittestandreset64(&Mask, Index);
                    NOS_MEMORY_DESCRIPTOR* Mem = &PhysicalMem->Groups[c].MemoryDescriptors[Index];

                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    }
}