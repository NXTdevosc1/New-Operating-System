#include <loader.h>

UINT64 SystemSpaceOffset = 0;

UINT64 SystemSpaceBaseAddress = 0x800000000000 | ((UINT64)1 << 48);

typedef struct _PAGE_TABLE_ENTRY {
    UINT64 Present : 1;
    UINT64 ReadWrite : 1;
    UINT64 UserSupervisor : 1;
    UINT64 Ignore0 : 1; // PWT
    UINT64 Ignore1 : 1; // PCD
    UINT64 Accessed : 1;
    UINT64 Dirty : 1;
    UINT64 SizePAT : 1; // PAT for 4KB Pages
    UINT64 Global : 1;
    UINT64 Ignored0 : 3;
    UINT64 PhysicalAddr : 36; // In 2-MB Pages BIT 0 Set to PAT
    UINT64 Ignored1 : 15;
    UINT64 Rsv : 1; // XD Bit
}  PTENTRY, *RFPTENTRY;

RFPTENTRY* NosKernelPageTable = NULL;

EFI_MEMORY_DESCRIPTOR* BlNewEntry = NULL;
EFI_MEMORY_DESCRIPTOR* SourceHeap = NULL; // Largest heap in the EFI_MEMORY_MAP

extern EFI_LOADED_IMAGE* LoadedImage;
extern UINTN _NumSystemPages;
void BlInitPageTable() {
    NosKernelPageTable = (RFPTENTRY*)BlAllocateOnePage();
    ZeroAlignedMemory((void*)NosKernelPageTable, 0x1000);
    
    for(UINT64 i = 0;i<NosInitData.MemoryCount;i++) {
        EFI_MEMORY_DESCRIPTOR* Desc = (EFI_MEMORY_DESCRIPTOR*)((char*)NosInitData.MemoryMap + i * NosInitData.MemoryDescriptorSize);

        // Set virtualstart to physical start
        Desc->VirtualStart = Desc->PhysicalStart;
        if(Desc->Type == EfiLoaderData || Desc->Type == EfiLoaderCode || Desc->Type == EfiConventionalMemory) {
            // Identity map the memory
            BlMapMemory((void*)Desc->PhysicalStart, (void*)Desc->PhysicalStart, Desc->NumberOfPages, PM_WRITEACCESS);
        } else {
            BlMapMemory((void*)Desc->PhysicalStart, (void*)Desc->PhysicalStart, Desc->NumberOfPages, PM_WRITEACCESS);
        }
    }
    // // Map frame buffer
    // BlMapMemory(NosInitData.FrameBuffer.BaseAddress, NosInitData.FrameBuffer.BaseAddress, (NosInitData.FrameBuffer.FbSize >> 21) + 1, PM_LARGE_PAGES | PM_WRITEACCESS);
    // // Map the APIC
    // BlMapMemory((void*)0xfee00000, (void*)0xfee00000, 1, PM_WRITEACCESS | PM_LARGE_PAGES);
    // Map the kernel to system space

	BlMapMemory((void*)SystemSpaceBaseAddress, NosInitData.NosPhysicalBase, _NumSystemPages >> 9, PM_LARGE_PAGES | PM_WRITEACCESS | PM_GLOBAL);
    BlMapMemory((void*)NosInitData.FrameBuffer.BaseAddress, NosInitData.FrameBuffer.BaseAddress, Convert2MBPages(NosInitData.FrameBuffer.FbSize >> 12), PM_LARGE_PAGES | PM_WRITEACCESS);
    __asm__ volatile ("mov %0, %%cr3" :: "r"(NosKernelPageTable));
}




void BlMapMemory(
    void* VirtualAddress,
    void* PhysicalAddress,
    UINT64 Count,
    UINT64 Flags
){
    if(((UINT64)VirtualAddress & 0xFFFF800000000000) == 0xFFFF800000000000) {
        VirtualAddress = (void*)(((UINT64)VirtualAddress & ~0xFFFF000000000000) | ((UINT64)1 << 48));
    }
    RFPTENTRY Pml4Entry = (RFPTENTRY)((UINT64)NosKernelPageTable & ~(0xFFF)), PdpEntry = NULL, PdEntry = NULL, PtEntry = NULL;
    UINT64 TmpVirtualAddr = (UINT64)VirtualAddress >> 12;
    UINT64 TmpPhysicalAddr = (UINT64)PhysicalAddress >> 12;

    UINT64 Pml4Index = 0, PdpIndex = 0, PdIndex = 0, PtIndex = 0;
    UINT64 EntryAddr = 0;
    PTENTRY ModelEntry = { 0 };

    ModelEntry.Present = TRUE;
    if (Flags & PM_WRITEACCESS) {
        ModelEntry.ReadWrite = TRUE;
    }
    if (Flags & PM_GLOBAL) ModelEntry.Global = TRUE;

    UINT64 IncVaddr = 1;
    if(Flags & PM_LARGE_PAGES) {
        ModelEntry.SizePAT = 1; // 2MB Pages
        IncVaddr = 0x200;
    }

    for(UINT64 i = 0;i<Count;i++, TmpPhysicalAddr+=IncVaddr, TmpVirtualAddr+=IncVaddr){

        PtIndex = TmpVirtualAddr & 0x1FF;
        PdIndex = (TmpVirtualAddr >> 9) & 0x1FF;
        PdpIndex = (TmpVirtualAddr >> 18) & 0x1FF;
        Pml4Index = (TmpVirtualAddr >> 27) & 0x1FF;


        if(!Pml4Entry[Pml4Index].Present){
            EntryAddr = (UINT64)BlAllocateOnePage();
            Pml4Entry[Pml4Index].PhysicalAddr = EntryAddr >> 12;
            Pml4Entry[Pml4Index].Present = 1;
            Pml4Entry[Pml4Index].ReadWrite = 1;
            // Pml4Entry[Pml4Index].UserSupervisor = 1;
            

            ZeroAlignedMemory((void*)EntryAddr, 0x1000);
        }else EntryAddr = (UINT64)Pml4Entry[Pml4Index].PhysicalAddr << 12;
        
        PdpEntry = (RFPTENTRY)EntryAddr;

        if(!PdpEntry[PdpIndex].Present){
            EntryAddr = (UINT64)BlAllocateOnePage();

            PdpEntry[PdpIndex].PhysicalAddr = EntryAddr >> 12;
            PdpEntry[PdpIndex].Present = 1;
            PdpEntry[PdpIndex].ReadWrite = 1;
            // PdpEntry[PdpIndex].UserSupervisor = 1;

            ZeroAlignedMemory((void*)EntryAddr, 0x1000);
        }else EntryAddr = (UINT64)PdpEntry[PdpIndex].PhysicalAddr << 12;
        PdEntry = (RFPTENTRY)EntryAddr;

        if(Flags & PM_LARGE_PAGES) {
            *&PdEntry[PdIndex] = ModelEntry;
            PdEntry[PdIndex].PhysicalAddr = TmpPhysicalAddr;
        } else {
            if(PdEntry[PdIndex].SizePAT) continue; // This is not a page directory
            if(!PdEntry[PdIndex].Present){
                EntryAddr = (UINT64)BlAllocateOnePage();
                PdEntry[PdIndex].PhysicalAddr = EntryAddr >> 12;
                PdEntry[PdIndex].Present = 1;
                PdEntry[PdIndex].ReadWrite = 1;
                // PdEntry[PdIndex].UserSupervisor = 1;

                ZeroAlignedMemory((void*)EntryAddr, 0x1000);

            }else EntryAddr = (UINT64)PdEntry[PdIndex].PhysicalAddr << 12;

            PtEntry = (RFPTENTRY)EntryAddr;

            *&PtEntry[PtIndex] = ModelEntry;

            PtEntry[PtIndex].PhysicalAddr = TmpPhysicalAddr;
        }
    }
}