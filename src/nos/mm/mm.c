#include <nos/nos.h>

void* KeMainThread = (void*)(UINT64)-1; // for tests

// Mm Find Allocated Memory Continuation Entry
NOS_MEMORY_DESCRIPTOR* MmFindAMContinuationEntry(void* PhysicalStart) {
    NOS_MEMORY_LINKED_LIST* mem = NosInitData->NosMemoryMap;
    unsigned long Index;
    while(mem) {
        for(int i = 0;i<0x40;i++) {
            UINT64 m = mem->AllocatedMemoryDescriptorsMask[i];
            while(_BitScanForward64(&Index, m)) {
                _bittestandreset64(&m, Index);
                NOS_MEMORY_DESCRIPTOR* desc = &mem->Groups[i].MemoryDescriptors[Index];
                if(((UINT64)desc->PhysicalAddress + (desc->NumPages << 12)) == (UINT64)PhysicalStart) {
                    return desc;
                }
            }
        }
        mem = mem->Next;
    }
    return NULL;
}

NOS_MEMORY_DESCRIPTOR* MmCreateMemoryDescriptor(
    void* PhysicalAddress,
    UINT32 Attributes,
    UINT64 NumPages
) {

    NOS_MEMORY_DESCRIPTOR* desc;
    if(Attributes & MM_DESCRIPTOR_ALLOCATED) {
        desc = MmFindAMContinuationEntry(PhysicalAddress);
        if(desc) {
            desc->NumPages += NumPages;
            return desc;
        }
    }
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
                desc = &mem->Groups[Index].MemoryDescriptors[Index2];
                desc->PhysicalAddress = PhysicalAddress;
                desc->Attributes = Attributes;
                desc->NumPages = NumPages;

                if(Attributes & MM_DESCRIPTOR_ALLOCATED) {
                    _bittestandset64(&mem->AllocatedMemoryDescriptorsMask[Index], Index2);
                }
                KeMutexRelease(KeMainThread, &mem->GroupsMutex[Index]);
                return desc;
            }
            KeMutexRelease(KeMainThread, &mem->GroupsMutex[Index]);
        }
        if(!mem->Next) {
            SerialLog("Allocating New NOS_MEMORY_LINKED_LIST");
            if(NERROR(
            MmAllocatePhysicalMemory(
                MM_ALLOCATE_WITHOUT_DESCRIPTOR,
                ConvertToPages(sizeof(NOS_MEMORY_LINKED_LIST)),
                &mem->Next
            )
            )) {
                SerialLog("Failed to allocate new NOS_MEMORY_LINKED_LIST");
                // TODO : Crash
                while(1);
            }
            *mem->Next = (NOS_MEMORY_LINKED_LIST){0};
            // Create the descriptor for the heap
            MmCreateMemoryDescriptor(mem->Next, 0, ConvertToPages(sizeof(NOS_MEMORY_LINKED_LIST)));
            SerialLog("Allocation Success");
        };
        mem = mem->Next;
    }
}

NOS_VIRTUAL_MEMORY_DESCRIPTOR* VmFindLinkableEntry(
    PROCESS* Process,
    void* VirtualStart,
    UINT64 PageFlags
) {
    NOS_VIRTUAL_MEMORY_LIST* Vm = &Process->VirtualMemory;
    for(;;Vm = Vm->Next) {
        UINT index;
        UINT64 Mask = Vm->Present;
        while(_BitScanForward64(&index, Mask)) {
            _bittestandreset64(&Mask, index);
            if(((UINT64)Vm->Vm[index].VirtualStart + Vm->Vm[index].NumBytes) == (UINT64)VirtualStart) 
            {
                if((Vm->Vm[index].PageFlags & PAGE_FLAGS_UNTOLERABLE_BITMASK) !=
                (PageFlags & PAGE_FLAGS_UNTOLERABLE_BITMASK)
                ) return NULL; // Can't link the two page chains

                return Vm->Vm + index;
            }
        }
        if(!Vm->Next) break;
    }
    return NULL;
}

NOS_VIRTUAL_MEMORY_DESCRIPTOR* VmCreateDescriptor(
    PROCESS* Process,
    void* VirtualStart,
    UINT64 NumPages,
    UINT64 PageFlags
) {
    NOS_VIRTUAL_MEMORY_DESCRIPTOR* Desc = VmFindLinkableEntry(Process, VirtualStart, PageFlags);
    if(Desc) {
        Desc->NumPages+=NumPages;
        if(PageFlags & PAGE_2MB) Desc->NumBytes += (NumPages << 21);
        else Desc->NumBytes += (NumPages << 12);
        return Desc;
    }
    NOS_VIRTUAL_MEMORY_LIST* Vm = &Process->VirtualMemory;
    if(Process->VmListSearchStart) Vm = Process->VmListSearchStart;
    UINT Index;
    UINT64 CpuFlags;
    for(;;Vm = Vm->Next) {
        if(Vm->Present != (UINT64)-1) {
            CpuFlags = KeAcquireSpinLock(&Vm->SpinLock);
            if(_BitScanForward64(&Index, ~Vm->Present)) {
                _bittestandset64(&Vm->Present, Index);
                Desc = &Vm->Vm[Index];
                // if(NERROR(MmAllocatePhysicalMemory(0, ConvertToPages(sizeof(NOS_HEAP_TREE)), &Desc->Heaps))) {
                //     SerialLog("MmCreateVMDesc : Allocation error!");
                //     while(1);
                // }
                // ObjZeroMemory(Desc->Heaps);
                Desc->VirtualStart = VirtualStart;
                Desc->NumPages = NumPages;
                if(PageFlags & PAGE_2MB) Desc->NumBytes = NumPages << 21;
                else Desc->NumBytes = NumPages << 12;
                Desc->PageFlags = PageFlags;
                Desc->Parent = Vm;
                Process->VmListSearchStart = Vm;
                KeReleaseSpinLock(&Vm->SpinLock, CpuFlags);
                return Desc;
            }
            KeReleaseSpinLock(&Vm->SpinLock, CpuFlags);
        }
        if(!Vm->Next) {
            if(NERROR(MmAllocatePhysicalMemory(0, ConvertToPages(sizeof(NOS_VIRTUAL_MEMORY_LIST)), &Vm->Next))) {
                SerialLog("MmCreateVMDesc : Allocation error!");
                while(1);
            }
            Vm->Next->Present = 0;
        }
    }
}

void VmRemoveDescriptor(
    NOS_VIRTUAL_MEMORY_DESCRIPTOR* Descriptor
) {
    _interlockedbittestandreset64(&Descriptor->Parent->Present, ((UINT64)Descriptor - (UINT64)Descriptor->Parent) / sizeof(NOS_VIRTUAL_MEMORY_DESCRIPTOR));
}