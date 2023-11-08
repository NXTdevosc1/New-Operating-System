#include <nos/nos.h>
#include <nos/processor/internal.h>
#include <nos/mm/physmem.h>

HMIMAGE _NosHeapImages[4];

HMIMAGE *_NosPhysical1GBImage = _NosHeapImages;     // huge page allocations
HMIMAGE *_NosPhysical2MBImage = _NosHeapImages + 1; // large page allocations
HMIMAGE *_NosPhysical4KBImage = _NosHeapImages + 2; // page allocations
HMIMAGE *_NosKernelHeap = _NosHeapImages + 3;

KPAGEHEADER StaticPageHeaders[0x1000];

void *MemBase;

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

    MemBase = KeReserveExtendedSpace(NosInitData->TotalPagesCount);

    // UINT64 LenPageHeaders = sizeof(KPAGEHEADER) * NosInitData->TotalPagesCount;

    KDebugPrint("Best region address %x Length %u bytes", BestMem->PhysicalAddress, BestMem->NumPages << 12);

    if (BestMem->NumPages < (1 << 18))
    {
        KDebugPrint("Failed to startup the kernel, must atleast have 1GB of continuous memory.");
        while (1)
            __halt();
    }
    // Prepare heap manager
    UINT64 T1G = oHmpCreateImage(_NosPhysical1GBImage, (1 << 30), BestMem->NumPages >> 18);
    UINT64 T2M = oHmpCreateImage(_NosPhysical2MBImage, (1 << 21), 0x200);
    UINT64 T4K = oHmpCreateImage(_NosPhysical4KBImage, 0x1000, 0x200);

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
    // _PageHeaders = (void *)(imgs + T1G + T2M + T4K);
    // _Memset128A_32(_PageHeaders, 0, LenPageHeaders >> 4);

    KDebugPrint("IMGS %x", imgs);
    KDebugPrint("Kernel boot memory : Used %d bytes , free : %d bytes, total %d bytes", AllocatedMemory, FreeMemory, AllocatedMemory + FreeMemory);

    oHmpInitImage(_NosPhysical1GBImage, imgs);
    oHmpInitImage(_NosPhysical2MBImage, imgs + T1G);
    oHmpInitImage(_NosPhysical4KBImage, imgs + T1G + T2M);

    _NosPhysical4KBImage->User.HigherImage = _NosPhysical2MBImage;
    _NosPhysical2MBImage->User.HigherImage = _NosPhysical1GBImage;

    PhysicalMem = NosInitData->NosMemoryMap;

    NOS_MEMORY_DESCRIPTOR cmem = {0}; // copy mem
    BOOLEAN usecmem2 = FALSE;
    NOS_MEMORY_DESCRIPTOR cmem2 = {0}; // copy mem

    UINT16 StaticIndex = 0;

    // Calculate and rearrange
    do
    {
        for (int c = 0; c < 0x40; c++)
        {
            if (StaticIndex > 0xFF0) // limit of static memory entries
                break;
            if (PhysicalMem->Groups[c].Present)
            {
                UINT64 Mask = PhysicalMem->Groups[c].Present;
                while (_BitScanForward64(&Index, Mask))
                {
                    _bittestandreset64(&Mask, Index);
                    NOS_MEMORY_DESCRIPTOR *Mem = &PhysicalMem->Groups[c].MemoryDescriptors[Index];
                    if (!(Mem->Attributes & MM_DESCRIPTOR_ALLOCATED))
                    {
                        memcpy(&cmem, Mem, sizeof *Mem);
                        KDebugPrint("ADDR %x-%x NUMPG %x VADDR %x", cmem.PhysicalAddress, (UINT64)cmem.PhysicalAddress + (cmem.NumPages << 12), cmem.NumPages);
                        if (cmem.NumPages >= (HUGEPAGE + ExcessBytes((UINT64)cmem.PhysicalAddress >> 12, HUGEPAGE)))
                        {
                            char *HugeStart = (char *)AlignForward((UINT64)cmem.PhysicalAddress, HUGEPAGE << 12);

                            UINT64 NumHugePages = (cmem.NumPages - ExcessBytes(((UINT64)cmem.PhysicalAddress >> 12), HUGEPAGE)) >> 18;
                            UINT64 RemainingLeft = (HugeStart - (char *)cmem.PhysicalAddress) >> 12;
                            UINT64 RemainingRight = cmem.NumPages - (RemainingLeft + (NumHugePages << 18));

                            KPAGEHEADER *hdr = StaticPageHeaders + StaticIndex;
                            hdr->Header.Address = (UINT64)HugeStart;
                            hdr->Header.FirstBlock = 1;
                            hdr->Header.NextBlock = NULL;

                            oHmpSet(_NosPhysical1GBImage, &hdr->Header, NumHugePages);
                            StaticIndex++;

                            KDebugPrint("%d HUGE PAGES at %x REML %d at %x REMR %d at %x", NumHugePages, HugeStart, RemainingLeft, cmem.PhysicalAddress, RemainingRight, (UINT64)cmem.PhysicalAddress + ((RemainingLeft + (NumHugePages << 18) << 12)));

                            if (RemainingRight)
                            {
                                cmem2.NumPages = RemainingRight;
                                cmem2.PhysicalAddress = (HugeStart + (NumHugePages << 30));
                                usecmem2 = TRUE;
                            }
                            if (RemainingLeft)
                            {
                                cmem.NumPages = RemainingLeft;
                            }
                            else
                                cmem.NumPages = 0;
                        }
                    cmem2goback:
                        if (cmem.NumPages >= (LARGEPAGE + ExcessBytes((UINT64)cmem.PhysicalAddress >> 12, LARGEPAGE)))
                        {
                            char *LargeStart = (char *)AlignForward((UINT64)cmem.PhysicalAddress, LARGEPAGE << 12);
                            UINT64 NumLargePages = (cmem.NumPages - ExcessBytes(((UINT64)cmem.PhysicalAddress >> 12), LARGEPAGE)) >> 9;

                            UINT64 RemainingLeft = (LargeStart - (char *)cmem.PhysicalAddress) >> 12;
                            UINT64 RemainingRight = cmem.NumPages - (RemainingLeft + (NumLargePages << 9));

                            KPAGEHEADER *hdr = StaticPageHeaders + StaticIndex;
                            hdr->Header.Address = (UINT64)LargeStart;
                            hdr->Header.FirstBlock = 1;
                            hdr->Header.NextBlock = NULL;

                            oHmpSet(_NosPhysical2MBImage, &hdr->Header, NumLargePages);
                            StaticIndex++;

                            KDebugPrint("%d LARGE PAGES at %x REML %d at %x REMR %d at %x", NumLargePages, LargeStart, RemainingLeft, cmem.PhysicalAddress, RemainingRight, (UINT64)cmem.PhysicalAddress + ((RemainingLeft + (NumLargePages << 9) << 12)));
                            if (RemainingRight)
                            {
                                cmem2.NumPages = RemainingRight;
                                cmem2.PhysicalAddress = (LargeStart + (NumLargePages << 21));
                                usecmem2 = TRUE;
                            }
                            if (RemainingLeft)
                            {
                                cmem.NumPages = RemainingLeft;
                            }
                            else
                                cmem.NumPages = 0;
                        }
                        if (cmem.NumPages)
                        {
                            KPAGEHEADER *hdr = StaticPageHeaders + StaticIndex;
                            hdr->Header.Address = (UINT64)cmem.PhysicalAddress;
                            hdr->Header.FirstBlock = 1;
                            hdr->Header.NextBlock = NULL;

                            oHmpSet(_NosPhysical4KBImage, &hdr->Header, cmem.NumPages);
                            StaticIndex++;
                            KDebugPrint("%d Individual 4KB pages at 0x%x-0x%x", cmem.NumPages, cmem.PhysicalAddress, ((UINT64)cmem.PhysicalAddress + (cmem.NumPages << 12)));
                        }

                        if (usecmem2)
                        {
                            memcpy(&cmem, &cmem2, sizeof cmem);
                            usecmem2 = FALSE;
                            goto cmem2goback;
                        }

                        KDebugPrint("_________________");
                    }
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    } while (PhysicalMem);

    KDebugPrint("NOS Optimized memory system initialized successfully.");

    for (;;)
    {
        PVOID p = MmRequestContiguousPages(0, 0x10);
        KDebugPrint("Returned 1 page %x", p);
    }
}