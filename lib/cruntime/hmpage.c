
/*
 HEAP MANAGER AREA INTERFACE

 Performs optimized sorting to process a heap request in the fastest way possible
*/

#include <nosdef.h>
#include <intrin.h>
#include <hmdef.h>

typedef struct _ASAREA
{
    UINT64 CompleteAddress; // address * unitlength + base address
    UINT64 AllocatedLength;
    UINT64 FreeLength;
    void *Next, *Previous;
} ASAREA;

#pragma pack(push, 1)
typedef struct _SIZETREE
{
    UINT16 Count;       // for alignment
    UINT64 Available[]; // bitmap/address to heap
} SIZETREE;
#pragma pack(pop)

typedef struct _HMIMAGE
{
    HMUSERHEADER User;
    UINT64 UnitLength;

    ULONG MaxBit;
    ULONG ubs; // unit bitshift count

    UINT64 epb, baddr, mskaddr, bbmp; // entries per entry level, entries per address level

    UINT64 NumEntries; // num entries last level, subsequent levels
    UINT64 NumQwords;
    UINT64 llNumQwords; // Num last level qwords

    // Bitmap
    UINT64 sel, sbl; // size entries level, size bitmap levels

    UINT64 TotalMemory;

    SIZETREE *Bases[4];
} HMIMAGE;
UINT64 HMAPI oHmpCreateImage(
    HMIMAGE *as,
    UINT64 Alignment,
    UINT64 MaxLengthInAlignedUnits)
{
    ObjZeroMemory(as);
    as->UnitLength = Alignment;

    _BitScanReverse64(&as->MaxBit, MaxLengthInAlignedUnits);
    _BitScanReverse64(&as->ubs, Alignment);
    as->User.AlignShift = as->ubs;
    as->User.Alignment = Alignment;

    as->NumEntries = 1 << (as->MaxBit % 3);
    as->epb = 1 << (as->MaxBit / 3);
    as->baddr = as->MaxBit % 3;
    as->mskaddr = as->NumEntries - 1;

    as->bbmp = as->MaxBit / 3;
    as->NumQwords = AlignForward(as->epb, 64) / 64;
    as->llNumQwords = AlignForward(as->NumEntries, 64) / 64;
    // size tree length should be power of 2
    as->sbl = as->NumQwords * 8 + sizeof(SIZETREE);
    as->sel = as->NumEntries * 8 + sizeof(SIZETREE) + as->llNumQwords * 8;
    KDebugPrint("BADDR %d BBMP %d ENTRIESPERBMP %d NUMADDRENTRIES %d QWORDSPERBMP %d ADDRTREELEN %d BMPTREELEN %d",
                as->baddr, as->bbmp, as->epb, as->NumEntries, as->NumQwords, as->sel, as->sbl);

    UINT64 BitMemory = as->sbl + as->sbl * as->epb + as->sbl * as->epb * as->epb;
    UINT64 ValueMemory = as->sel * as->epb * as->epb * as->epb;
    UINT64 CoveredLength = as->NumEntries * as->epb * as->epb * as->epb;
    KDebugPrint("Bit mem %x val mem %x Covered mem in bytes %x", BitMemory, ValueMemory, CoveredLength * Alignment);

    as->TotalMemory = AlignForward(BitMemory + ValueMemory, 0x1000);
    return as->TotalMemory;
}

void HMAPI oHmpInitImage(
    HMIMAGE *as, // address space
    char *Mem)
{
    as->Bases[0] = (void *)(Mem);
    as->Bases[1] = (void *)(Mem + as->sbl);
    as->Bases[2] = (void *)((char *)as->Bases[1] + as->epb * as->sbl);
    as->Bases[3] = (void *)((char *)as->Bases[2] + as->epb * as->epb * as->sbl);

    XmemsetAligned(Mem, 0, as->TotalMemory >> 7); // uses 128 byte blocks
}

void HMAPI oHmpSet(HMIMAGE *as, HMHEADER *Mem, UINT64 TheoriticalLength)
{
    UINT64 i3 = TheoriticalLength & (as->NumEntries - 1);
    UINT64 i2 = (TheoriticalLength >> as->baddr) & (as->epb - 1);
    UINT64 i1 = (TheoriticalLength >> (as->baddr + as->bbmp)) & (as->epb - 1);
    UINT64 i0 = (TheoriticalLength >> (as->baddr + as->bbmp * 2)) & (as->epb - 1);

    SIZETREE *s1 = ((SIZETREE *)((char *)as->Bases[1] + i0 * as->sbl));
    SIZETREE *s2 = ((SIZETREE *)((char *)as->Bases[2] + i0 * as->sbl * as->epb + i1 * as->sbl));
    SIZETREE *s3 = ((SIZETREE *)((char *)as->Bases[3] + i0 * as->sel * as->epb * as->epb + i1 * as->sel * as->epb + i2 * as->sel));

    _bittestandset64(as->Bases[0]->Available + (i0 >> 6), i0 & 0x3F); // not requiring count value
    if (!_bittestandset64(s1->Available + (i1 >> 6), i1 & 0x3F))
        s1->Count++;
    if (!_bittestandset64(s2->Available + (i2 >> 6), i2 & 0x3F))
        s2->Count++;

    if (_bittestandset64(s3->Available + (i3 >> 6), i3 & 0x3F))
    {
        HMHEADER *hdr = (void *)(s3->Available[as->llNumQwords + i3]);
        // set last block
        hdr->PrevOrLastBlock->NextBlock = Mem;
        Mem->PrevOrLastBlock = hdr->PrevOrLastBlock;
        hdr->PrevOrLastBlock = Mem;
        Mem->FirstBlock = 0;
    }
    else
    {
        s3->Available[as->llNumQwords + i3] = (UINT64)Mem;
        Mem->FirstBlock = 1;
        Mem->PrevOrLastBlock = Mem;
        s3->Count++;
    }
    Mem->NextBlock = NULL;
}

void HMAPI oHmpDelete(HMIMAGE *as, HMHEADER *Mem, UINT64 TheoriticalLength)
{
    UINT64 i3 = TheoriticalLength & (as->NumEntries - 1);
    UINT64 i2 = (TheoriticalLength >> as->baddr) & (as->epb - 1);
    UINT64 i1 = (TheoriticalLength >> (as->baddr + as->bbmp)) & (as->epb - 1);
    UINT64 i0 = (TheoriticalLength >> (as->baddr + as->bbmp * 2)) & (as->epb - 1);

    SIZETREE *s1 = ((SIZETREE *)((char *)as->Bases[1] + i0 * as->sbl));
    SIZETREE *s2 = ((SIZETREE *)((char *)as->Bases[2] + i0 * as->sbl * as->epb + i1 * as->sbl));
    SIZETREE *s3 = ((SIZETREE *)((char *)as->Bases[3] + i0 * as->sel * as->epb * as->epb + i1 * as->sel * as->epb + i2 * as->sel));

    if (Mem->FirstBlock)
    {
        if (Mem->PrevOrLastBlock != Mem)
        {
            Mem->NextBlock->PrevOrLastBlock = Mem->PrevOrLastBlock;
            Mem->NextBlock->FirstBlock = 1;
            s3->Available[as->llNumQwords + i3] = (UINT64)Mem->NextBlock;
        }
        else
        {
            _bittestandreset64(s3->Available + (i3 >> 6), i3 & 0x3F);
            s3->Available[as->llNumQwords + i3] = 0;
            s3->Count--;
            if (!s3->Count)
            {
                _bittestandreset64(s2->Available + (i2 >> 6), i2 & 0x3F);
                s2->Count--;
                if (!s2->Count)
                {
                    _bittestandreset64(s1->Available + (i1 >> 6), i1 & 0x3F);
                    s1->Count--;
                    if (!s1->Count)
                    {
                        _bittestandreset64(as->Bases[0]->Available + (i0 >> 6), i0 & 0x3F); // not requiring count value
                    }
                }
            }
        }
    }
    else
    {
        if (Mem->NextBlock)
        {
            Mem->NextBlock->PrevOrLastBlock = Mem->PrevOrLastBlock;
        }
        Mem->PrevOrLastBlock->NextBlock = Mem->NextBlock;
        if (Mem->PrevOrLastBlock->PrevOrLastBlock == Mem)
        {
            Mem->PrevOrLastBlock->PrevOrLastBlock = Mem->PrevOrLastBlock;
        }
    }
}

BOOLEAN HMAPI oHmpLookup(HMIMAGE *as)
{
    if (as->User.BestHeap.Block)
    {
        oHmpDelete(as, as->User.BestHeap.Block, as->User.BestHeap.StartupLength);
        oHmpSet(as, as->User.BestHeap.Block, as->User.BestHeap.RemainingLength);
    }
    ULONG i0, i1, i2, i3;
    // LVL0
    for (ULONG i = as->NumQwords - 1;; i--)
    {
        if (_BitScanReverse64(&i0, as->Bases[0]->Available[i]))
        {
            i0 = (i << 6) | i0;
            goto lvl1;
        }
        if (i == 0)
            break;
    }

    return FALSE;
    // LVL1
lvl1:

    SIZETREE *s1 = ((SIZETREE *)((char *)as->Bases[1] + i0 * as->sbl));
    for (ULONG i = as->NumQwords - 1;; i--)
    {
        if (_BitScanReverse64(&i1, s1->Available[i]))
        {
            i1 = (i << 6) | i1;
            break;
        }
        if (i == 0)
            break;
    }
    // LVL2
    SIZETREE *s2 = ((SIZETREE *)((char *)as->Bases[2] + i0 * as->sbl * as->epb + i1 * as->sbl));

    for (ULONG i = as->NumQwords - 1;; i--)
    {
        if (_BitScanReverse64(&i2, s2->Available[i]))
        {
            i2 = (i << 6) | i2;
            break;
        }
        if (i == 0)
            break;
    }
    // LVL3
    SIZETREE *s3 = ((SIZETREE *)((char *)as->Bases[3] + i0 * as->sel * as->epb * as->epb + i1 * as->sel * as->epb + i2 * as->sel));
    for (ULONG i = as->llNumQwords - 1;; i--)
    {
        if (_BitScanReverse64(&i3, s3->Available[i]))
        {
            PHMHEADER Block = (PHMHEADER)s3->Available[as->llNumQwords + (i << 6) + i3];
            BOOLEAN Ret = TRUE;
            // KDebugPrint("found");
            if (Block == as->User.BestHeap.Block)
                Ret = FALSE;
            else
            {
                as->User.BestHeap.Block = Block;
                as->User.BestHeap.StartupLength = i3 + i2 * as->NumEntries + i1 * as->epb * as->NumEntries + i0 * as->epb * as->epb * as->NumEntries;
                // KDebugPrint("length %d blocks - addr : 0x%x", as->User.BestHeap.StartupLength, as->User.BestHeap.Block->Address);
                as->User.BestHeap.RemainingLength = as->User.BestHeap.StartupLength;
            }
            return Ret;
        }
        if (i == 0)
            break;
    }

    KDebugPrint("Heap Manager : HM_ERR");
    KeRaiseException(STATUS_OUT_OF_BOUNDS);
}

PHMHEADER HMAPI oHmpGet(HMIMAGE *as, UINT64 TheoriticalLength)
{
    return (PHMHEADER)(((SIZETREE *)((char *)as->Bases[3] + ((TheoriticalLength >> as->baddr) * as->sel)))->Available[as->llNumQwords + (TheoriticalLength & as->mskaddr)]);
}
