#include <nos/mm/physmem.h>

NSTATUS KRNLAPI MmAllocatePhysicalMemory(UINT64 Flags, UINT64 NumPages, void** Ptr) {
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
    UINT64 Align = 0x1000;
    if(Flags & ALLOCATE_2MB_ALIGNED_MEMORY) {
        Align = 0x200000;
        NumPages <<= 9;
    }
    if(Flags & ALLOCATE_1GB_ALIGNED_MEMORY) {
        Align = 0x40000000;
        NumPages <<= 18;
    }
    if(NosInitData->TotalPagesCount - NosInitData->AllocatedPagesCount < NumPages) return STATUS_OUT_OF_MEMORY;
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
                    ExcessBytes(Mem->PhysicalAddress, Align) + NumBytes)
                     < 0xFFFFFFFF && Mem->NumPages >= ((ExcessBytes(Mem->PhysicalAddress, Align) + NumBytes) >> 12)) {
                        // Found Available Memory
                        *Ptr = (void*)((UINT64)Mem->PhysicalAddress + ExcessBytes(Mem->PhysicalAddress, Align));
                        void* addr = *Ptr;
                        if(ExcessBytes(Mem->PhysicalAddress, Align)) {
                            MmCreateMemoryDescriptor(Mem->PhysicalAddress, Mem->Attributes, ExcessBytes(Mem->PhysicalAddress, Align) >> 12);
                        }
                        if(!(Flags & MM_ALLOCATE_WITHOUT_DESCRIPTOR)) {
                            MmCreateMemoryDescriptor(addr, MM_DESCRIPTOR_ALLOCATED, NumPages);
                        }
                        (UINT64)Mem->PhysicalAddress = (UINT64)addr + NumBytes;
                        Mem->NumPages -= NumPages + (ExcessBytes(Mem->PhysicalAddress, Align) >> 12);
                        
                        if(!Mem->NumPages) {
                            // Remove descriptor
                            Mem->Attributes = 0;
                            _bittestandreset64(&PhysicalMem->Groups[c].Present, Index);
                            _bittestandreset64(&PhysicalMem->Full, c);
                        }
                        else _bittestandreset(&Mem->Attributes, MM_DESCRIPTOR_BUSY);
                        
                        
                        
                        _interlockedadd64(&NosInitData->AllocatedPagesCount, NumPages);

                        return STATUS_SUCCESS;
                    }
                    _bittestandreset(&Mem->Attributes, MM_DESCRIPTOR_BUSY);
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    }
    SerialLog("OUT_OF mem");
    return STATUS_OUT_OF_MEMORY;
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
    if(NosInitData->TotalPagesCount - NosInitData->AllocatedPagesCount < NumPages) return STATUS_OUT_OF_MEMORY;

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
                    if(Mem->NumPages >= ((ExcessBytes(Mem->PhysicalAddress, Align) + NumBytes) >> 12)) {
                        *Ptr = (void*)((UINT64)Mem->PhysicalAddress + ExcessBytes(Mem->PhysicalAddress, Align));
                        void* addr = *Ptr;
                        // Found Available Memory
                        if(ExcessBytes(Mem->PhysicalAddress, Align)) {
                            MmCreateMemoryDescriptor(Mem->PhysicalAddress, Mem->Attributes, ExcessBytes(Mem->PhysicalAddress, Align) >> 12);
                        }
                        if(!(Flags & MM_ALLOCATE_WITHOUT_DESCRIPTOR)) {
                            MmCreateMemoryDescriptor((void*)((UINT64)addr), MM_DESCRIPTOR_ALLOCATED, NumPages);
                        }
                        (UINT64)Mem->PhysicalAddress = (UINT64)addr + NumBytes;
                        Mem->NumPages -= (NumPages + (ExcessBytes(Mem->PhysicalAddress, Align) >> 12));
                        _bittestandreset(&Mem->Attributes, MM_DESCRIPTOR_BUSY);
                        _interlockedadd64(&NosInitData->AllocatedPagesCount, NumPages);

                        return STATUS_SUCCESS;
                    }
                    
                    _bittestandreset(&Mem->Attributes, MM_DESCRIPTOR_BUSY);

                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    }
    return STATUS_OUT_OF_MEMORY;
}

// TODO : Free does not link properly

BOOLEAN KRNLAPI MmFreePhysicalMemory(
    IN void* PhysicalMemory,
    IN UINT64 NumPages
) {
    // UNIMPLEMENTED
    return TRUE;
    NOS_MEMORY_LINKED_LIST* mem = NosInitData->NosMemoryMap;
    unsigned long Index;
    UINT64 _paddr = (UINT64)PhysicalMemory;
    while(mem) {
        for(int i = 0;i<0x40;i++) {
            UINT64 m = mem->AllocatedMemoryDescriptorsMask[i];
            while(_BitScanForward64(&Index, m)) {
                _bittestandreset64(&m, Index);
                NOS_MEMORY_DESCRIPTOR* desc = &mem->Groups[i].MemoryDescriptors[Index];
                if(desc->NumPages < NumPages) continue;
                UINT64 addr = (UINT64)desc->PhysicalAddress;
                UINT64 endaddr = addr + (desc->NumPages << 12);
                if(addr <= _paddr && endaddr > _paddr) {
                    SerialLog("free found");
                    // Entry found
                    if(addr == _paddr) {
                        (char*)desc->PhysicalAddress += (NumPages << 12);
                        desc->NumPages -= NumPages;
                        if(!desc->NumPages) {
                            _interlockedbittestandreset64(&mem->Groups[i].Present, Index);
                        }
                    } else if(endaddr == (_paddr + (NumPages << 12))){
                        desc->NumPages -= NumPages;
                        if(!desc->NumPages) {
                            SerialLog("MMFPS: BUG0");
                            while(1);
                        }
                    } else {
                        // eg. 0x1000-0x4000 < 0x2000 && (0x1000 + 0x3000) > 0x2000
                        // Split entries
                        UINT64 PageIndex = (_paddr - addr) >> 12;
                        MmCreateMemoryDescriptor((char*)(addr + ((PageIndex + NumPages) << 12)), desc->Attributes, desc->NumPages - NumPages - PageIndex);
                        desc->NumPages = PageIndex;
                    }
                    MmCreateMemoryDescriptor(PhysicalMemory, 0, NumPages);
                    NosInitData->AllocatedPagesCount -= NumPages;
                    return TRUE;
                }
            }
        }
        mem = mem->Next;
    }
    return FALSE;
}