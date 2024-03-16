#include <nos/nos.h>
#include <nos/mm/internal.h>

MEMREQUEST MemRequests[] = {
    iRequestPhysicalMemory,
    iRequestVirtualMemory};

PVOID KRNLAPI
KRequestMemory(
    UINT RequestType,
    PEPROCESS Process,
    UINT Flags,
    UINT64 Length)
{

    return MemRequests[RequestType](Process, Flags, Length);
}

PVOID KRNLAPI KFreeMemory(
    PVOID Address,
    UINT64 Length,
    UINT Type)
{
    return NULL;
}

// Mm Find Allocated Memory Continuation Entry
NOS_MEMORY_DESCRIPTOR *MmFindAMContinuationEntry(void *PhysicalStart)
{
    NOS_MEMORY_LINKED_LIST *mem = NosInitData->NosMemoryMap;
    unsigned long Index;

    while (mem)
    {
        for (int i = 0; i < 0x40; i++)
        {
            UINT64 m = mem->AllocatedMemoryDescriptorsMask[i];
            while (_BitScanForward64(&Index, m))
            {
                _bittestandreset64(&m, Index);
                NOS_MEMORY_DESCRIPTOR *desc = &mem->Groups[i].MemoryDescriptors[Index];
                if (((UINT64)desc->PhysicalAddress + (desc->NumPages << 12)) == (UINT64)PhysicalStart)
                {
                    return desc;
                }
            }
        }
        mem = mem->Next;
    }
    return NULL;
}

NOS_MEMORY_DESCRIPTOR *MmFindFMContinuationEntry(void *PhysicalStart)
{
    NOS_MEMORY_LINKED_LIST *mem = NosInitData->NosMemoryMap;
    unsigned long Index;
    while (mem)
    {
        for (int i = 0; i < 0x40; i++)
        {
            UINT64 m = mem->Groups[i].Present;
            while (_BitScanForward64(&Index, m))
            {
                _bittestandreset64(&m, Index);
                NOS_MEMORY_DESCRIPTOR *desc = &mem->Groups[i].MemoryDescriptors[Index];
                if ((desc->Attributes & MM_DESCRIPTOR_ALLOCATED))
                    continue;
                if (((UINT64)desc->PhysicalAddress + (desc->NumPages << 12)) == (UINT64)PhysicalStart)
                {
                    return desc;
                }
            }
        }
        mem = mem->Next;
    }
    return NULL;
}

NOS_MEMORY_DESCRIPTOR *MmCreateMemoryDescriptor(
    void *PhysicalAddress,
    UINT32 Attributes,
    UINT64 NumPages)
{
    NOS_MEMORY_DESCRIPTOR *desc;
    if (Attributes & MM_DESCRIPTOR_ALLOCATED)
    {
        desc = MmFindAMContinuationEntry(PhysicalAddress);
        if (desc)
        {
            desc->NumPages += NumPages;
            return desc;
        }
        // KDebugPrint("ahm 6");
    }
    else
    {
        desc = MmFindFMContinuationEntry(PhysicalAddress);
        if (desc)
        {
            desc->NumPages += NumPages;
            SerialLog("fm found");
            return desc;
        }
        // SerialLog("MmCMD: ERR1");
    }
    NOS_MEMORY_LINKED_LIST *mem = NosInitData->NosMemoryMap;
    unsigned long Index = 0;
    for (;;)
    {
        if (_BitScanForward64(&Index, ~mem->Full))
        {
            // KDebugPrint("ahm 55 %d %x %x %d", NumPages, ~mem->Full, &Index, _BitScanForward64(&Index, ~mem->Full));
            // while(1);
            ExMutexWait(NULL, &mem->GroupsMutex[Index], 0);
            unsigned long Index2;
            if (_BitScanForward64(&Index2, ~mem->Groups[Index].Present))
            {
                _bittestandset64(&mem->Groups[Index].Present, Index2);
                if (mem->Groups[Index].Present == (UINT64)-1)
                {
                    _bittestandset64(&mem->Full, Index);
                }

                desc = &mem->Groups[Index].MemoryDescriptors[Index2];
                desc->PhysicalAddress = PhysicalAddress;
                desc->Attributes = Attributes;
                desc->NumPages = NumPages;

                if (Attributes & MM_DESCRIPTOR_ALLOCATED)
                {
                    _bittestandset64(&mem->AllocatedMemoryDescriptorsMask[Index], Index2);
                }
                ExMutexRelease(NULL, &mem->GroupsMutex[Index]);
                return desc;
            }
            ExMutexRelease(NULL, &mem->GroupsMutex[Index]);
            // KDebugPrint("ahm 69 %d %x %d", NumPages, ~mem->Full, _BitScanForward64(&Index, ~mem->Full));
            // while(1);
        }

        if (!mem->Next)
        {
            SerialLog("Allocating New NOS_MEMORY_LINKED_LIST");
            if (NERROR(
                    MmAllocatePhysicalMemory(
                        MM_ALLOCATE_WITHOUT_DESCRIPTOR,
                        ConvertToPages(sizeof(NOS_MEMORY_LINKED_LIST)),
                        &mem->Next)))
            {
                SerialLog("Failed to allocate new NOS_MEMORY_LINKED_LIST");
                // TODO : Crash
                while (1)
                    __halt();
            }
            ObjZeroMemory(mem->Next);
            // Create the descriptor for the heap
            MmCreateMemoryDescriptor(mem->Next, 0, ConvertToPages(sizeof(NOS_MEMORY_LINKED_LIST)));
            SerialLog("Allocation Success");
        };
        mem = mem->Next;
    }
}
