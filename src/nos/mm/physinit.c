#include <nos/nos.h>
#include <nos/processor/internal.h>
#include <hm.h>

HMIMAGE _NosHeapImages[4];

HMIMAGE *_NosPhysical1GBImage = _NosHeapImages;     // huge page allocations
HMIMAGE *_NosPhysical2MBImage = _NosHeapImages + 1; // large page allocations
HMIMAGE *_NosPhysical4KBImage = _NosHeapImages + 2; // page allocations
HMIMAGE *_NosKernelHeap = _NosHeapImages + 3;

void KRNLAPI __KiClearScreen(UINT Color);

void NOSINTERNAL KiPhysicalMemoryManagerInit()
{

    NOS_MEMORY_DESCRIPTOR *BestMem = NULL; // In pages

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
                        if (!BestMem || Mem->NumPages > BestMem->NumPages)
                        {
                            BestMem = Mem;
                        }
                        FreeMemory += Mem->NumPages * 0x1000;
                    }
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    } while (PhysicalMem);

    NosInitData->AllocatedPagesCount = AllocatedMemory >> 12;
    NosInitData->TotalPagesCount = (AllocatedMemory + FreeMemory) >> 12;

    KDebugPrint("Best region address %x Length %u bytes", BestMem->PhysicalAddress, BestMem->NumPages << 12);

    KDebugPrint("Kernel boot memory : Used %d bytes , free : %d bytes, total %d bytes", AllocatedMemory, FreeMemory, AllocatedMemory + FreeMemory);

    if (BestMem->NumPages < (1 << 18))
    {
        KDebugPrint("Failed to startup the kernel, must atleast have 1GB of continuous memory.");
        while (1)
            __halt();
    }
    // Prepare heap manager
    UINT64 T1G = CreateMemorySpace(_NosPhysical1GBImage, (1 << 30), BestMem->NumPages >> 18);
    UINT64 T2M = CreateMemorySpace(_NosPhysical2MBImage, (1 << 21), BestMem->NumPages >> 9);
    UINT64 T4K = CreateMemorySpace(_NosPhysical4KBImage, 0x1000, BestMem->NumPages);

    // Can contain upto (4KB-1) of splitted chunk, otherwise use page frame allocations to allocate chunks

    UINT64 Total = T1G + T2M + T4K;

    KDebugPrint("T4G %x T2M %x T4K %x", T1G, T2M, T4K);
    char *imgs;
    if ((NERROR(MmAllocatePhysicalMemory(ALLOCATE_2MB_ALIGNED_MEMORY, AlignForward(Total, 0x200000) >> 21, &imgs))))
    {
        KDebugPrint("Failed to startup the kernel, no enough memory to initialize the heap manager.");
        while (1)
            __halt();
    }

    KDebugPrint("IMGS %x", imgs);

    InitMemorySpace(_NosPhysical1GBImage, imgs);
    InitMemorySpace(_NosPhysical2MBImage, imgs + T1G);
    InitMemorySpace(_NosPhysical4KBImage, imgs + T1G + T2M);
}