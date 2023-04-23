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

#define BytesToAlign(Value, Align) (((UINT64)Value & (Align - 1)) ? (Align - ((UINT64)Value & (Align - 1))) : (0))

// Allocates memory below 4gb
NSTATUS KRNLAPI MmAllocateLowMemory(
    UINT64 Flags, UINT64 NumPages, void** Ptr
) {
    NOS_MEMORY_LINKED_LIST* PhysicalMem = NosInitData->NosMemoryMap;
    unsigned long Index;
    UINT64 Align = 0x1000;
    if(Flags & ALLOCATE_2MB_ALIGNED_MEMORY) {
        Align = 0x200000;
        NumPages <<= 9;
    }
    if(Flags & ALLOCATE_1GB_ALIGNED_MEMORY) {
        Align = 0x40000000;
        NumPages <<= 18;
    }
    UINT64 NumBytes = NumPages << 12;
    while(PhysicalMem) {
        for(int c = 0;c<0x40;c++) {
            if(PhysicalMem->Groups[c].Present) {
                UINT64 Mask = PhysicalMem->Groups[c].Present;
                while(_BitScanForward64(&Index, Mask)) {
                    _bittestandreset64(&Mask, Index);
                    NOS_MEMORY_DESCRIPTOR* Mem = &PhysicalMem->Groups[c].MemoryDescriptors[Index];
                    if(Mem->Attributes & MM_DESCRIPTOR_ALLOCATED) continue;
                    if(_interlockedbittestandset(&Mem->Attributes, MM_DESCRIPTOR_BUSY)) continue;

                    if(((UINT64)Mem->PhysicalAddress + 
                    BytesToAlign(Mem->PhysicalAddress, Align) + NumBytes)
                     < 0xFFFFFFFF && Mem->NumPages >= ((BytesToAlign(Mem->PhysicalAddress, Align) + NumBytes) >> 12)) {
                        // Found Available Memory
                        if(BytesToAlign(Mem->PhysicalAddress, Align)) {
                            MmCreateMemoryDescriptor(Mem->PhysicalAddress, Mem->Attributes, BytesToAlign(Mem->PhysicalAddress, Align) >> 12);
                        }
                        MmCreateMemoryDescriptor((void*)((UINT64)Mem->PhysicalAddress + BytesToAlign(Mem->PhysicalAddress, Align)), MM_DESCRIPTOR_ALLOCATED, NumPages);
                        *Ptr = (void*)((UINT64)Mem->PhysicalAddress + BytesToAlign(Mem->PhysicalAddress, Align));
                        (UINT64)Mem->PhysicalAddress += BytesToAlign(Mem->PhysicalAddress, Align) + NumBytes;
                        Mem->NumPages -= NumPages + (BytesToAlign(Mem->PhysicalAddress, Align) >> 12);
                        _bittestandreset(&Mem->Attributes, MM_DESCRIPTOR_BUSY);
                        return STATUS_SUCCESS;
                    }
                    _bittestandreset(&Mem->Attributes, MM_DESCRIPTOR_BUSY);
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    }
    return STATUS_UNSUFFICIENT_MEMORY;
}



// Allocates memory above 4gb
/*
No need to split or check if a heap surpasses 4GB barrier while have a physical base of less than 4GB
All firmwares declare memory above 4GB in a separated heap
*/
NSTATUS KRNLAPI MmAllocateHighMemory(
    UINT64 Flags, UINT64 NumPages, void** Ptr
) {
    NOS_MEMORY_LINKED_LIST* PhysicalMem = NosInitData->NosMemoryMap;
    unsigned long Index;
    UINT64 Align = 0x1000;
    if(Flags & ALLOCATE_2MB_ALIGNED_MEMORY) {
        Align = 0x200000;
        NumPages <<= 9;
    }
    if(Flags & ALLOCATE_1GB_ALIGNED_MEMORY) {
        Align = 0x40000000;
        NumPages <<= 18;
    }
    UINT64 NumBytes = NumPages << 12;
    while(PhysicalMem) {
        for(int c = 0;c<0x40;c++) {
            if(PhysicalMem->Groups[c].Present) {
                UINT64 Mask = PhysicalMem->Groups[c].Present;
                while(_BitScanForward64(&Index, Mask)) {
                    _bittestandreset64(&Mask, Index);
                    NOS_MEMORY_DESCRIPTOR* Mem = &PhysicalMem->Groups[c].MemoryDescriptors[Index];
                    if(Mem->Attributes & MM_DESCRIPTOR_ALLOCATED ||
                     (UINT64)Mem->PhysicalAddress < 0x100000000) continue;
                    if(_interlockedbittestandset(&Mem->Attributes, MM_DESCRIPTOR_BUSY)) continue;
                    
                    if(Mem->NumPages >= ((BytesToAlign(Mem->PhysicalAddress, Align) + NumBytes) >> 12)) {
                        // Found Available Memory
                        if(BytesToAlign(Mem->PhysicalAddress, Align)) {
                            MmCreateMemoryDescriptor(Mem->PhysicalAddress, Mem->Attributes, BytesToAlign(Mem->PhysicalAddress, Align) >> 12);
                        }
                        MmCreateMemoryDescriptor((void*)((UINT64)Mem->PhysicalAddress + BytesToAlign(Mem->PhysicalAddress, Align)), MM_DESCRIPTOR_ALLOCATED, NumPages);
                        *Ptr = (void*)((UINT64)Mem->PhysicalAddress + BytesToAlign(Mem->PhysicalAddress, Align));
                        (UINT64)Mem->PhysicalAddress += BytesToAlign(Mem->PhysicalAddress, Align) + NumBytes;
                        Mem->NumPages -= NumPages + (BytesToAlign(Mem->PhysicalAddress, Align) >> 12);
                        _bittestandreset(&Mem->Attributes, MM_DESCRIPTOR_BUSY);
                        return STATUS_SUCCESS;
                    }
                    
                    _bittestandreset(&Mem->Attributes, MM_DESCRIPTOR_BUSY);

                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    }
    return STATUS_UNSUFFICIENT_MEMORY;
}