#include <nos/processor/internal.h>

typedef struct _PAGE_TABLE_ENTRY {
    UINT64 Present : 1;
    UINT64 ReadWrite : 1;
    UINT64 UserSupervisor : 1;
    UINT64 PWT : 1; // PWT
    UINT64 PCD : 1; // PCD
    UINT64 Accessed : 1;
    UINT64 Dirty : 1;
    UINT64 SizePAT : 1; // PAT for 4KB Pages
    UINT64 Global : 1;
    UINT64 Ignored0 : 3;
    UINT64 PhysicalAddr : 36; // In 2-MB Pages BIT 0 Set to PAT
    UINT64 Ignored1 : 15;
    UINT64 ExecuteDisable : 1; // XD Bit
}  PTENTRY, *RFPTENTRY;

#define SetPageEntry(_pentry, _entry) {*(UINT64*)(&_pentry) |= _entry;}

NSTATUS KRNLAPI KeMapPhysicalMemory(
    PROCESS* Process,
    IN void* _PhysicalAddress,
    IN void* _VirtualAddress,
    IN UINT64 NumPages,
    IN PAGE_FLAGS PageFlags,
    IN UINT CacheType
) {

    // TODO : Check if pages are already mapped

    RFPTENTRY Pml4 = Process->PageTable, Pdp, Pd, Pt;
    UINT64 PhysicalAddress = (UINT64)_PhysicalAddress & ~0xFFF;
    UINT64 VirtualAddress = (UINT64)_VirtualAddress & ~0xFFF;

    if((VirtualAddress & 0xFFFF800000000000) == 0xFFFF800000000000) {
        VirtualAddress &= ~0xFFFF000000000000;
        VirtualAddress |= ((UINT64)1 << 48);
    }

    // Setup Model Entry
    UINT64 ModelEntry = ((UINT64)1 /*Present Flag*/);
    {
        RFPTENTRY __m = (RFPTENTRY)&ModelEntry;
        if(PageFlags & PAGE_WRITE_ACCESS) __m->ReadWrite = 1;
        if(PageFlags & PAGE_EXECUTE_DISABLE) __m->ExecuteDisable = 1;
        if(PageFlags & PAGE_GLOBAL) __m->Global = 1;
        if(PageFlags & PAGE_USER) __m->UserSupervisor = 1;
    }
    UINT64 OrEntry = ModelEntry & ~(((UINT64)1 << 63) | ((UINT64)1<<8) /*Remove orying with XD|GB Bits*/);


    for(;;) {
        UINT64 Pti = (VirtualAddress >> 12) & 0x1FF;
        UINT64 Pdi = (VirtualAddress >> 21) & 0x1FF;
        UINT64 Pdpi = (VirtualAddress >> 30) & 0x1FF;
        UINT64 Pml4i = (VirtualAddress >> 39) & 0x1FF;

        if(!Pml4[Pml4i].Present) {
            if(NERROR(KeAllocatePhysicalMemory(0, 1, (void**)&Pml4[Pml4i]))) {
                SerialLog("KeMapPhysicalMemory : AllocatePage Error.");
                while(1) __halt();
            }
            Pdp = *(void**)&Pml4[Pml4i];
            ZeroMemory(Pdp, 0x1000);
            SetPageEntry(Pml4[Pml4i], ModelEntry);
        } else {
            SetPageEntry(Pml4[Pml4i], OrEntry);
            Pdp = (void*)(Pml4[Pml4i].PhysicalAddr << 12);
        }
        if(!Pdp[Pdpi].Present) {
            if(NERROR(KeAllocatePhysicalMemory(0, 1, (void**)&Pdp[Pdpi]))) {
                SerialLog("KeMapPhysicalMemory : AllocatePage Error.");
                while(1) __halt();
            }
            Pd = *(void**)&Pdp[Pdpi];
            ZeroMemory(Pd, 0x1000);
            SetPageEntry(Pdp[Pdpi], ModelEntry);
        } else {
            SetPageEntry(Pdp[Pdpi], OrEntry);
            Pd = (void*)(Pdp[Pdpi].PhysicalAddr << 12);
        }

        if(PageFlags & PAGE_2MB) {
            for(UINT64 i = Pdi;i<0x200;i++, NumPages--, VirtualAddress+=0x200000, PhysicalAddress+=0x200000) {
                if(!NumPages) return STATUS_SUCCESS;
                SetPageEntry(Pd[i], ModelEntry | PhysicalAddress | (1 << 7));
            }
        } else {
            if(!Pd[Pdi].Present) {
                if(NERROR(KeAllocatePhysicalMemory(0, 1, (void**)&Pd[Pdi]))) {
                    SerialLog("KeMapPhysicalMemory : AllocatePage Error.");
                    while(1) __halt();
                }
                Pt = *(void**)&Pd[Pdi];
                ZeroMemory(Pt, 0x1000);
                SetPageEntry(Pd[Pdi], ModelEntry);
            } else {
                SetPageEntry(Pd[Pdi], OrEntry);
                Pt = (void*)(Pd[Pdi].PhysicalAddr << 12);
            }
            for(UINT64 i = Pti;i<0x200;i++, NumPages--, VirtualAddress+=0x1000, PhysicalAddress+=0x1000) {
                if(!NumPages) return STATUS_SUCCESS;
                SetPageEntry(Pt[i], ModelEntry | PhysicalAddress);
            }
        }

    }
}