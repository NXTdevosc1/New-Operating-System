#include <nos/nos.h>
#include <nos/processor/internal.h>
#include <nos/mm/physmem.h>

HMIMAGE _NosHeapImages[4] = {0};

HMIMAGE *_NosPhysical1GBImage = _NosHeapImages + HugePageSize;   // huge page allocations
HMIMAGE *_NosPhysical2MBImage = _NosHeapImages + LargePageSize;  // large page allocations
HMIMAGE *_NosPhysical4KBImage = _NosHeapImages + NormalPageSize; // page allocations
HMIMAGE *_NosKernelHeap = _NosHeapImages + 3;

KPAGEHEADER StaticPageHeaders[0x1000];

void *MemBase;

void KRNLAPI __KiClearScreen(UINT Color);

PVOID __fastcall _KHeapAllocatePages(HMIMAGE *Image, UINT64 Count)
{
    KDebugPrint("Src alloc");
    PVOID p = MmRequestContiguousPagesNoDesc(_NosPhysical4KBImage, Count);
    if (!p)
    {
        Count++; // First page used as heap descriptor
        KDebugPrint("Allocating additional pages");
        UINT64 bigpages = AlignForward(Count, 0x200) >> 9;

        p = MmRequestContiguousPagesNoDesc(_NosPhysical2MBImage, bigpages);
        if (!p)
        {
            KDebugPrint("Allocating 1GB additional pages");

            bigpages = AlignForward(bigpages, 0x200) >> 9;
            p = MmRequestContiguousPagesNoDesc(_NosPhysical1GBImage, bigpages);
            if (!p)
                return NULL;

            UINT64 Rempages = (bigpages << 18) - Count;
            UINT64 RemLpages = Rempages >> 9;
            Rempages = ((RemLpages << 9) - Rempages);

            HMBLK *Blk = p;
            KPAGEHEADER *Hdr = (void *)(Blk + 1);
            Blk->Addr = (UINT64)Hdr >> 4;
            PVOID NewP = (char *)p + 0x1000;
            if (RemLpages)
            {
                Hdr->Header.Address = (UINT64)p + ((Count + Rempages) << 12);
                KDebugPrint("2MB ADDR %x REM %x", Hdr->Header.Address, RemLpages);
                oHmpSet(_NosPhysical2MBImage, &Hdr->Header, RemLpages);
                Hdr++;
            }
            if (Rempages)
            {
                Hdr->Header.Address = (UINT64)p + (Count << 12);
                KDebugPrint("4KB ADDR %x REM %x", Hdr->Header.Address, Rempages);
                oHmpSet(_NosPhysical4KBImage, &Hdr->Header, Rempages);
            }
            else if (!RemLpages)
                NewP = p; // 4KB will be wasted (TODO : Decrease allocation amount)

            if (Rempages || RemLpages)
            {
                HMBLK *BlkFree = (void *)(Hdr + 1);
                BlkFree->Addr = (UINT64)(BlkFree + 1);
                oHmbSet(Image, BlkFree, 0x1000 - (BlkFree->Addr - (UINT64)Blk));
                BlkFree->Addr >>= 4;
            }

            return NewP;
        }
        else
        {
            HMBLK *Blk = p;
            ((char *)p) += 0x1000;
            KPAGEHEADER *Hdr = (void *)(Blk + 1);
            Blk->Addr = (UINT64)Hdr >> 4;
            Hdr->Header.Address = (UINT64)p;
            HMBLK *BlkFree = (void *)(Hdr + 1);
            BlkFree->Addr = (UINT64)(BlkFree + 1);
            oHmbSet(Image, BlkFree, 0x1000 - (BlkFree->Addr - (UINT64)Blk));
            BlkFree->Addr >>= 4;
        }
    }

    // TODO : May return more than requested
    return p;
}

void __fastcall _KHeapFreePages(HMIMAGE *Image, PVOID Mem, UINT64 Count)
{
    KDebugPrint("WARNING : _KHeapFreePages Not Implemented");
}

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

    oHmbInitImage(_NosKernelHeap, _KHeapAllocatePages, _KHeapFreePages);

    KDebugPrint("NOS Optimized memory system initialized successfully.");

    __KiClearScreen(0xFF);

    for (UINT64 i = 0;; i++)
    {
        PVOID p = MmRequestContiguousPages(0, 1);
        if (!p)
        {
            KDebugPrint("%u pages have been allocated, efficiency (total clean allocations): %u bytes, total free memory at startup : %u bytes",
                        i, i << 12, (NosInitData->TotalPagesCount - NosInitData->AllocatedPagesCount) << 12);
            break;
        }
        // KDebugPrint("Returned 1 page %x", p);
    }

    __KiClearScreen(0xFFFF);

    while (1)
        __halt();
}