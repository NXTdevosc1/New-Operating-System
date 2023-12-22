#include <nos/nos.h>
#include <nos/processor/internal.h>
#include <nos/mm/physmem.h>

HMIMAGE _NosHeapImages[2] = {0};
HMIMAGE *_NosPhysicalMemoryImage = _NosHeapImages;
HMIMAGE *_NosKernelHeap = _NosHeapImages + 1;

char *PageBase = NULL;
KPAGEHEADER *PageHeaders = NULL;
UINT64 MaxAddress;
void *MemBase;

void KRNLAPI __KiClearScreen(UINT Color);

PVOID __fastcall _KHeapAllocatePages(HMIMAGE *Image, UINT64 Count)
{
    KDebugPrint("Src alloc");
    // PVOID p = MmRequestContiguousPagesNoDesc(_NosPhysical4KBImage, Count);

    // TODO : May return more than requested
    return NULL;
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
    MaxAddress = 0;
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
                    if ((UINT64)Mem->PhysicalAddress + (Mem->NumPages << 12) > MaxAddress)
                        MaxAddress = ((UINT64)Mem->PhysicalAddress + (Mem->NumPages << 12));
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    } while (PhysicalMem);

    NosInitData->AllocatedPagesCount = AllocatedMemory >> 12;
    NosInitData->TotalPagesCount = (AllocatedMemory + FreeMemory) >> 12;

    // Those headers will be mapped as a shadow physical address copy : VirtualBase + PhysicalAddress
    UINT64 LenPageHeaders = sizeof(KPAGEHEADER) * NosInitData->TotalPagesCount;

    KDebugPrint("Best region address %x Length %u bytes", BestMem->PhysicalAddress, BestMem->NumPages << 12);

    if (BestMem->NumPages < (1 << 18))
    {
        KDebugPrint("Failed to startup the kernel, must atleast have 1GB of continuous memory.");
        while (1)
            __halt();
    }
    // Prepare heap manager

    char *VmmBuffer;
    // TODO : Support 4/5 Level physical memory
    UINT64 MemLen = ConvertToPages(AlignForward(VmmLevelLength(4), 0x1000) + 0x1000 + LenPageHeaders);
    if ((NERROR(MmAllocatePhysicalMemory(0, MemLen, &VmmBuffer))))
    {
        KDebugPrint("Failed to startup the kernel, no enough memory to initialize the memory manager.");
        while (1)
            __halt();
    }
    ZeroMemory(VmmBuffer, MemLen);

    KDebugPrint("Allocated %x bytes, MAX_ADDR %x, TOTAL_PAGES %x", MemLen << 12, MaxAddress, NosInitData->TotalPagesCount);

    // Prepare page descriptors
    PageBase = KeReserveExtendedSpace(NosInitData->TotalPagesCount << 12);

    PageHeaders = (KPAGEHEADER *)(VmmBuffer + AlignForward(VmmLevelLength(4), 0x1000) + 0x1000);

    PhysicalMem = NosInitData->NosMemoryMap;

    // Init Physical Allocator
    // TODO : Currently 3 Levels, supports only allocations < 512GB
    VmmCreate(_NosPhysicalMemoryImage, 4, VmmBuffer, sizeof(KPAGEHEADER));
    NOS_MEMORY_DESCRIPTOR cmem = {0}; // copy mem
    BOOLEAN usecmem2 = FALSE;
    NOS_MEMORY_DESCRIPTOR cmem2 = {0}; // copy mem

    KPAGEHEADER *hdr;
    char *CurrentPageAddress = PageBase;
    UINT64 CurrentPageNumber = 0;
    // Calculate and rearrange
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

                            if (NumHugePages > 0x1FF)
                            {
                                // ____________ TODO ___________________
                                KDebugPrint("HALTPOINT : KERNEL TODO! MAP PAGES HIGHER THAN 512 GB");
                                while (1)
                                    __halt();
                            }

                            for (UINT64 i = 0; i < NumHugePages << 18; i++)
                            {
                                hdr = PageHeaders + CurrentPageNumber + i;
                                hdr->Header.Address = ((UINT64)HugeStart >> 12) + i;
                            }

                            VmmInsert(VmmPageLevel(_NosPhysicalMemoryImage, 2), PageHeaders + CurrentPageNumber, NumHugePages);
                            CurrentPageNumber += NumHugePages << 18;

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

                            for (UINT64 i = 0; i < NumLargePages << 9; i++)
                            {
                                hdr = PageHeaders + CurrentPageNumber + i;
                                hdr->Header.Address = ((UINT64)LargeStart >> 12) + i;
                            }

                            VmmInsert(VmmPageLevel(_NosPhysicalMemoryImage, 1), PageHeaders + CurrentPageNumber, NumLargePages);
                            CurrentPageNumber += NumLargePages << 9;

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
                            for (UINT64 i = 0; i < cmem.NumPages; i++)
                            {
                                hdr = PageHeaders + CurrentPageNumber + i;
                                hdr->Header.Address = ((UINT64)cmem.PhysicalAddress >> 12) + i;
                            }

                            VmmInsert(VmmPageLevel(_NosPhysicalMemoryImage, 0), PageHeaders + CurrentPageNumber, cmem.NumPages);
                            CurrentPageNumber += cmem.NumPages;

                            KDebugPrint("%d Individual 4KB pages at 0x%x-0x%x", cmem.NumPages, cmem.PhysicalAddress, ((UINT64)cmem.PhysicalAddress + (cmem.NumPages << 12)));
                        }

                        if (usecmem2)
                        {
                            memcpy(&cmem, &cmem2, sizeof cmem);
                            usecmem2 = FALSE;
                            goto cmem2goback;
                        }

                        KDebugPrint("_________________");
                        // Map the pages
                    }
                }
            }
        }
        PhysicalMem = PhysicalMem->Next;
    } while (PhysicalMem);

    KDebugPrint("NOS Optimized memory system initialized successfully. Current page number %x, Total mem %d GB", CurrentPageNumber, CurrentPageNumber >> 18);

    __KiClearScreen(0xFF);

    for (UINT64 i = 0;; i++)
    {
        PVOID p = MmRequestContiguousPages(0, 1);
        if (!p)
        {
            KDebugPrint("LVL0 : %u pages have been allocated, %d GB, %d Bytes", i, i >> 18, i << 12);
            KDebugPrint("Returned 1 page %x", p);
            break;
        }
        // KDebugPrint("PAGE %x", p);
    }

    __KiClearScreen(0xFFFF);

    while (1)
        __halt();
}