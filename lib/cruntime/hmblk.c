#include <hmdef.h>
#include <mm.h>
#include <xmmintrin.h>

typedef struct _HMIMAGE
{
    HMUSERHEADER User;
    // UINT64 UnitLength;  Always 0x10
    HMALLOCATE_ROUTINE SrcAlloc;
    HMFREE_ROUTINE SrcFree;

    PHMBLK InitialBlock;
    UINT64 InitialLength;

    PHMBLK CurrentBlock;
    UINT8 CurrentLength;

    UINT32 BaseBitmap;
    UINT64 SubBmp[4];
    PHMBLK HeapArray[256];
} HMIMAGE;

void HMAPI oHmbInitImage(
    HMIMAGE *Image,
    HMALLOCATE_ROUTINE AllocateRoutine,
    HMFREE_ROUTINE FreeRoutine)
{
    ObjZeroMemory(Image);
    Image->SrcAlloc = AllocateRoutine;
    Image->SrcFree = FreeRoutine;
}

typedef struct
{
    UINT64 Used : 1;
    UINT64 Length : 8;
    UINT64 OffsetToFooter : 48;
} BLOCKHEADER;

typedef struct
{
    BLOCKHEADER *Header;
} BLOCKFOOTER;

PVOID HMAPI oHmbAllocate(
    HMIMAGE *Image,
    UINT64 Length)
{
    UINT64 TargetLength = Length + 1; // 8 Bytes on top, 8 bytes on bottom
    BLOCKHEADER *Blk;
    BLOCKFOOTER *Footer;
    if (TargetLength < 0x100 && (Image->CurrentLength >= TargetLength || (oHmbLookup(Image) && Image->InitialLength >= TargetLength)))
    {

        Blk = Image->CurrentBlock;
        Image->CurrentBlock += TargetLength;
        Image->CurrentLength -= TargetLength;
    }
    else
    {

        // Allocate pages and fill the rest
        Blk = Image->SrcAlloc(Image, AlignForward(TargetLength, 0x100) >> 8);
        if (!Blk)
            return NULL;
        if (TargetLength & 0xFF)
        {
            oHmbSet(Image, Blk + 1, ExcessBytes(TargetLength, 0x100));
            KDebugPrint("Remaining of %x mem 0x%x %x bytes", Blk - Length, Blk + 1, ExcessBytes(TargetLength, 0x100) << 4);
        }
    }
    Blk->Used = 1;
    Blk->Length = Length;
    Blk->OffsetToFooter = (Length << 1) - 1;

    Footer = Blk + Blk->OffsetToFooter;
    Footer->Header = Blk;
    return Blk + 1;
}

BOOLEAN HMAPI oHmbFree(HMIMAGE *Image, void *Ptr)
{
    BLOCKHEADER *Header = (BLOCKHEADER *)Ptr - 1;
    BLOCKHEADER *Prev = ((BLOCKFOOTER *)(Header - 1))->Header;

    const UINT64 vhdr = (UINT64)Header;
    if (!(vhdr & 0xFFF) && Header->Length >= 0x100)
    {
        KDebugPrint("FREE PAGE %x LEN %x", Header, Header->Length);
    }
    else if (!Prev->Used)
    {
        Prev->Length += Header->Length;
        if (Prev->Length >= 0x100)
        {
            KDebugPrint("FREE PAGE PREV %x HDR %x PREVLEN %x", Prev, Header, Prev->Length);
        }
    }
}
// PVOID HMAPI oHmbAllocate(
//     HMIMAGE *Image,
//     UINT64 Length)
// {
// Length += 2; // add 32 byte header
// UINT64 NumPages = Length >> 8;
// if (NumPages)
// { // exceeds 1 page
//     if (Length & 0xFF)
//     {
//     NewBlockAlloc:
//         NumPages++;
//         Length += 2; // another descriptor
//         if (Length > (NumPages << 8))
//         {
//             NumPages++;
//         }
//         UINT64 RemainingSize = (NumPages << 8) - Length;
//         PVOID Mem = Image->SrcAlloc(Image, NumPages);
//         if (!Mem)
//             return NULL;

//         PHMBLK Desc0 = Mem, Desc1 = (PHMBLK)((char *)Mem + ((Length - 2) << 4));
//         // Desc0->Addr = (UINT64)(Desc0 + 1) >> 4;
//         // Desc0->MainBlk = 1;
//         // Desc1->Addr = (UINT64)(Desc1 + 1) >> 4;
//         oHmbSet(Image, Desc1, RemainingSize);
//         if (!Image->BestHeap)
//         {
//             Image->BestHeap = Desc1;
//             Image->InitialLength = RemainingSize;
//             Image->CurrentLength = RemainingSize;
//         }
//         return Desc0 + 1;
//     }
//     PHMBLK Desc = Image->SrcAlloc(Image, NumPages);
//     // Desc->Addr = (UINT64)(Desc + 1) >> 4;
//     // Desc->MainBlk = 1;
//     return Desc + 1;
// }
// else
// {

//     if (Image->CurrentLength < Length)
//     {
//         if (oHmbLookup(Image))
//         {
//             if (Image->CurrentLength < Length)
//             {
//                 goto NewBlockAlloc;
//             }
//         }
//         else
//             goto NewBlockAlloc;
//     }
//     (char *)Image->BestHeap += Length;
//     PVOID Mem = (PVOID)(Image->BestHeap->Addr << 4);
//     Image->BestHeap->Addr += Length;
//     Image->CurrentLength -= Length;
//     return Mem;
// }
// }

void printbmp(HMIMAGE *img)
{
    KDebugPrint("______ PRINT_BITMAP_START : IMG %x ______", img);
    KDebugPrint("BASE 0x%x", img->BaseBitmap);
    for (int i = 0; i < 4; i++)
        KDebugPrint("Sub#%d 0x%x", i, img->SubBmp[i]);
    KDebugPrint("______ PRINT_BITMAP_END ______", img);
}

BOOLEAN HMAPI oHmbLookup(HMIMAGE *Image)
{
    ULONG Index, Index2;
    if (Image->InitialBlock != Image->CurrentBlock)
    {
        oHmbRemove(Image, Image->InitialBlock, Image->InitialLength);
        oHmbSet(Image, Image->CurrentBlock, Image->CurrentLength);
    }
    if (!_BitScanReverse(&Index, Image->BaseBitmap))
        return FALSE;

    _BitScanReverse64(&Index2, Image->SubBmp[Index]);
    PHMBLK Blk = Image->HeapArray[Index2 + (Index << 6)];

    Image->InitialBlock = Blk;
    Image->CurrentBlock = Blk;
    Image->InitialLength = Index2 + (Index << 6);
    Image->CurrentLength = Image->InitialLength;
    return TRUE;
}

void HMAPI oHmbSet(HMIMAGE *Image, PHMBLK Block, UINT8 Length /*in 16 Byte blocks*/)
{
    Image->BaseBitmap |= (1ULL << (Length >> 6));
    Block->Next = 0;

    if (_bittestandset64(Image->SubBmp + (Length >> 6), Length & 0x3F))
    {
        PHMBLK baseblk = Image->HeapArray[Length];
        ((PHMBLK)baseblk->PrevOrLast)->Next = (UINT64)Block; // set lastblock.next
        Block->PrevOrLast = baseblk->PrevOrLast;             // set block.previous
        baseblk->PrevOrLast = (UINT64)Block;                 // set firstblack.last
    }
    else
    {
        Block->PrevOrLast = (UINT64)Block;
        Image->HeapArray[Length] = Block;
    }
}

void HMAPI oHmbRemove(HMIMAGE *Image, PHMBLK Block, UINT8 Length)
{
    if (Block->PrevOrLast == (UINT64)Block)
    {
        // Starting heap, Only heap
        _bittestandreset64(Image->SubBmp + (Length >> 6), Length & 0x3F);
        if (!Image->SubBmp[Length >> 6])
        {
            _bittestandreset(&Image->BaseBitmap, Length >> 6);
        }
        Image->HeapArray[Length] = NULL;
    }
    else if (Image->HeapArray[Length] == Block)
    {
        // Starting heap
        ((PHMBLK)Block->Next)->PrevOrLast = Block->PrevOrLast; // set next with last block
        Image->HeapArray[Length] = (PVOID)Block->Next;         // set arr with next block
    }
    else
    {
        // Middle/Ending Heap
        ((PHMBLK)Block->PrevOrLast)->Next = Block->Next; // set previous block with value of next block
        if (Block->Next == 0)
        { // ending heap
            Image->HeapArray[Length]->PrevOrLast = Block->PrevOrLast;
        }
    }
}