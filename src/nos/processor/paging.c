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

#define SetPageEntry(_pentry, _entry) {*(UINT64*)(&_pentry) = _entry;}
#define OrPageEntry(_pentry, _entry) {*(UINT64*)(&_pentry) |= _entry;}

#define ClearPageEntry(_pentry) {*(UINT64*)(&_pentry) = 0;}
UINT64 StandardPageEntry = ((UINT64)0b111);

NSTATUS KRNLAPI KeMapVirtualMemory(
    PROCESS* Process,
    IN void* _PhysicalAddress,
    IN void* _VirtualAddress,
    IN UINT64 NumPages,
    IN UINT64 PageFlags,
    IN UINT CachePolicy
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
            ZeroMemory(Pdp + Pdpi, 0x1000 - (Pdpi << 3));
        } else {
            Pdp = (void*)(Pml4[Pml4i].PhysicalAddr << 12);
        }
        OrPageEntry(Pml4[Pml4i], StandardPageEntry);
        if(!Pdp[Pdpi].Present) {
            if(NERROR(KeAllocatePhysicalMemory(0, 1, (void**)&Pdp[Pdpi]))) {
                SerialLog("KeMapPhysicalMemory : AllocatePage Error.");
                while(1) __halt();
            }
            Pd = *(void**)&Pdp[Pdpi];
            ZeroMemory(Pd + Pdi, 0x1000 - (Pdi << 3));
        } else {
            Pd = (void*)(Pdp[Pdpi].PhysicalAddr << 12);
        }
        OrPageEntry(Pdp[Pdpi], StandardPageEntry);

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
                ZeroMemory(Pt + Pti, 0x1000 - (Pti << 3));
            } else {
                Pt = (void*)(Pd[Pdi].PhysicalAddr << 12);
            }
            OrPageEntry(Pd[Pdi], StandardPageEntry);
            for(UINT64 i = Pti;i<0x200;i++, NumPages--, VirtualAddress+=0x1000, PhysicalAddress+=0x1000) {
                if(!NumPages) return STATUS_SUCCESS;
                SetPageEntry(Pt[i], ModelEntry | PhysicalAddress);
            }
        }

    }
}


NSTATUS KRNLAPI KeUnmapVirtualMemory(
    IN PROCESS* Process,
    IN void* _VirtualAddress,
    IN OUT UINT64* _NumPages // Returns num pages left
) {
    // TODO : Check if pages are already mapped
    UINT64 NumPages = *_NumPages;
    RFPTENTRY Pml4 = Process->PageTable, Pdp, Pd, Pt;
    UINT64 VirtualAddress = (UINT64)_VirtualAddress & ~0xFFF;

    if((VirtualAddress & 0xFFFF800000000000) == 0xFFFF800000000000) {
        VirtualAddress &= ~0xFFFF000000000000;
        VirtualAddress |= ((UINT64)1 << 48);
    }

    for(;;) {
        UINT64 Pti = (VirtualAddress >> 12) & 0x1FF;
        UINT64 Pdi = (VirtualAddress >> 21) & 0x1FF;
        UINT64 Pdpi = (VirtualAddress >> 30) & 0x1FF;
        UINT64 Pml4i = (VirtualAddress >> 39) & 0x1FF;

        if(!Pml4[Pml4i].Present) {
            if(NumPages <= 0x8000000) {
                NumPages=0;
                *_NumPages = 0;
                return STATUS_SUCCESS;
            } else {
                NumPages-=      0x8000000;
                VirtualAddress+=0x8000000000;
                continue;
            }
        } else {
            Pdp = (void*)(Pml4[Pml4i].PhysicalAddr << 12);
        }
        if(!Pdp[Pdpi].Present) {
            if(NumPages <= 0x40000) {
                NumPages = 0;
                *_NumPages = 0;
                return STATUS_SUCCESS;
            } else {
                NumPages -= 0x40000;
                VirtualAddress += 0x40000000;
                continue;
            }
        } else {
            Pd = (void*)(Pdp[Pdpi].PhysicalAddr << 12);
        }

        if(!Pd[Pdi].Present) {
_gt0:
            if(NumPages <= 0x200) {
                NumPages = 0;
                *_NumPages = 0;
                return STATUS_SUCCESS;
            } else {
                NumPages -= 0x200;
                VirtualAddress += 0x200000;
                continue;
            }
        } else {
            if(Pd->SizePAT) {
                if(NumPages & 0x1FFFFF) {
                    *_NumPages = NumPages;
                    return STATUS_WRONG_PAGE_SIZE;
                }
                goto _gt0;
            }
            Pt = (void*)(Pd[Pdi].PhysicalAddr << 12);
        }

        if(!Pti && NumPages >= 0x200) {
            // TODO : Free Physical Memory
            ClearPageEntry(Pd[Pdi]);
            NumPages-=0x200;
            VirtualAddress+=0x200000;
            continue;
        } else {
            const register UINT max = (Pti + NumPages) < 0x200 ? (Pti + NumPages) : 0x200;
            for(UINT i = Pti;i<max;i++, NumPages--, VirtualAddress+=0x1000) {
                ClearPageEntry(Pt[i]);
            }
            if(!NumPages) {
                *_NumPages = 0;
                return STATUS_SUCCESS;
            }
        }

    }
}

BOOLEAN KRNLAPI KeCheckMemoryAccess(
    IN PROCESS* Process,
    IN void* _VirtualAddress,
    IN UINT64 NumBytes,
    IN OPT UINT64* _Flags
) {

    RFPTENTRY Pml4 = Process->PageTable, Pdp, Pd, Pt;
    UINT64 VirtualAddress = (UINT64)_VirtualAddress & ~0xFFF;
    UINT64 NumPages = NumBytes >> 12;
    if((UINT64)_VirtualAddress & 0x1FF) NumPages++;
    if((VirtualAddress & 0xFFFF800000000000) == 0xFFFF800000000000) {
        VirtualAddress &= ~0xFFFF000000000000;
        VirtualAddress |= ((UINT64)1 << 48);
    }

    UINT64 Flags = PAGE_WRITE_ACCESS | PAGE_USER | PAGE_GLOBAL;
    for(;;) {
        UINT64 Pti = (VirtualAddress >> 12) & 0x1FF;
        UINT64 Pdi = (VirtualAddress >> 21) & 0x1FF;
        UINT64 Pdpi = (VirtualAddress >> 30) & 0x1FF;
        UINT64 Pml4i = (VirtualAddress >> 39) & 0x1FF;
        if(!Pml4[Pml4i].Present) return FALSE;
        Pdp = (void*)(Pml4[Pml4i].PhysicalAddr << 12);
        if(!Pdp[Pdpi].Present) return FALSE;
        Pd = (void*)(Pdp[Pdpi].PhysicalAddr << 12);
        if(!Pd[Pdi].Present) return FALSE;
        if(Pd[Pdi].SizePAT) {
            Flags &= ((*(UINT64*)&Pd[Pdi]) | (PAGE_EXECUTE_DISABLE));
            Flags |= ((*(UINT64*)&Pd[Pdi]) & PAGE_EXECUTE_DISABLE);
            if(NumPages <= 0x200) {
                if(_Flags) *_Flags = Flags;
                return TRUE;
            } else {
                NumPages -= 0x200;
                VirtualAddress+=0x200000;
                continue;
            }
        }
        Pt = (void*)(Pd[Pdi].PhysicalAddr << 12);
        const register UINT max = (Pti + NumPages) < 0x200 ? (Pti + NumPages) : 0x200;

        for(UINT i = Pti;i<max;i++, VirtualAddress+=0x1000, NumPages--) {
            if(!Pt[i].Present) return FALSE;
            Flags &= ((*(UINT64*)&Pt[i]) | (PAGE_EXECUTE_DISABLE));
            Flags |= ((*(UINT64*)&Pt[i]) & PAGE_EXECUTE_DISABLE);
        }
        if(!NumPages) {
            if(_Flags) *_Flags = Flags;
            return TRUE;
        }
    }
}