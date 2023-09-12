#include <nos/nos.h>
#include <nos/processor/internal.h>

// Page table in the top of virtual address space
UINT64 *VPageTable = NULL;
UINT64 TotalVPageTableLength = 0;
void NOSINTERNAL KiPhysicalMemoryManagerInit()
{

    NOS_MEMORY_LINKED_LIST *PhysicalMem = NosInitData->NosMemoryMap;
    unsigned long Index = 0;
    UINT64 AllocatedMemory = 0, FreeMemory = 0;
    do
    {
        for (int c = 0; c < 0x40; c++)
        {
            if (PhysicalMem->Groups[c].Present)
            {
                UINT64 Mask = PhysicalMem->Groups[c].Present;
                while (_BitScanForward64(&Index, Mask))
                {
                    _bittestandreset64(&Mask, Index);
                    NOS_MEMORY_DESCRIPTOR *Mem = &PhysicalMem->Groups[c].MemoryDescriptors[Index];
                    if (Mem->Attributes & MM_DESCRIPTOR_ALLOCATED)
                    {
                        AllocatedMemory += Mem->NumPages * 0x1000;
                    }
                    else
                    {
                        FreeMemory += Mem->NumPages * 0x1000;
                    }
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    } while (PhysicalMem);
    KDebugPrint("Kernel boot memory : Used %d bytes , free : %d bytes, total %d bytes", AllocatedMemory, FreeMemory, AllocatedMemory + FreeMemory);

    NosInitData->AllocatedPagesCount = AllocatedMemory >> 12;
    NosInitData->TotalPagesCount = (AllocatedMemory + FreeMemory) >> 12;

    int NumPageLevels = 4;
    for (int i = 0; i < NumPageLevels; i++)
    {
        UINT64 val = 0x1000;
        for (int c = 0; c < i; c++)
            val = val * 0x200;
        TotalVPageTableLength += val;
    }
    TotalVPageTableLength >>= 12;

    // VPageTable = KeReserveExtendedSpace(TotalVPageTableLength);
    // KDebugPrint("Total Page Structure %u Pages or 0x%x . VP_ADDRESS %x", TotalVPageTableLength, TotalVPageTableLength, VPageTable);

    // // Now map all the pages into the address space
    // RFPTENTRY Pml4 = (RFPTENTRY)(__readcr3() & ~0xFFF);

    // HwMapVirtualMemory(Pml4, Pml4, VPageTable, 1, PAGE_WRITE_ACCESS, 0);

    // char* PdpOff = (char*)VPageTable + 0x1000;
    // for(UINT64 pdp = 0;pdp<0x200;pdp++,PdpOff+=0x1000) {
    //     if(!Pml4[pdp].Present) continue;

    //     RFPTENTRY Pdp = (RFPTENTRY)(Pml4[pdp].PhysicalAddr << 12);
    //     HwMapVirtualMemory(Pml4, Pdp, PdpOff, 1, PAGE_WRITE_ACCESS, 0);
    //     char* PdOff = (char*)VPageTable + 0x1000 + 0x1000 * 512;
    //     for(UINT64 pd = 0;pd<0x200;pd++,PdOff+=0x1000) {
    //         if(!Pdp[pd].Present) continue;
    //         RFPTENTRY Pd = (RFPTENTRY)(Pdp[pd].PhysicalAddr << 12);
    //         HwMapVirtualMemory(Pml4, Pd, PdOff, 1, PAGE_WRITE_ACCESS, 0);
    //         char* PtOff = (char*)VPageTable + 0x1000 + 0x1000 * 512 + 0x1000 * 512 * 512;
    //         for(UINT64 pt = 0;pt<0x200;pt++,PtOff+=0x1000) {
    //             if(!Pd[pt].Present || Pd[pt].SizePAT) continue;

    //             RFPTENTRY Pt = (RFPTENTRY)(Pd[pt].PhysicalAddr << 12);
    //             HwMapVirtualMemory(Pml4, Pt, PtOff, 1, PAGE_WRITE_ACCESS, 0);
    //         }
    //     }
    // }
}