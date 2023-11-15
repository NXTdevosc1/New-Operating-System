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

PVOID HMAPI oHmbAllocate(
    HMIMAGE *Image,
    UINT64 Length)
{
    Length++;
    PHMBLK Mem;
    if (Image->CurrentLength < Length &&
        (Length > 0xFF || !oHmbLookup(Image) || Image->CurrentLength < Length))
    {
        Mem = Image->SrcAlloc(Image, AlignForward(Length, 0x100) >> 8);
        if (!Mem)
            return NULL;
        if (Length & 0xFF)
        {
            PHMBLK Blk = Mem + Length;
            oHmbSet(Image, Blk, 0x100 - (Length & 0xFF));
        }
    }
    else
    {
        __m128i *NextHeap = (__m128i *)Image->CurrentBlock + Length;

        *NextHeap = *(__m128i *)Image->CurrentBlock;
        Mem = Image->CurrentBlock;
        Image->CurrentBlock = NextHeap;
        Image->CurrentLength -= Length;
    }
    Mem->Length = Length; // full block length
    return Mem + 1;
}

BOOLEAN HMAPI oHmbFree(HMIMAGE *Image, void *Ptr)
{
    PHMBLK Blk = ((PHMBLK)Ptr) - 1;
    PHMBLK Prev = Blk - Blk->Length, Next = Blk + Blk->Length;
    if (((UINT64)Blk & 0xFF0) && !Prev->Used)
    {
        Prev->Length += Blk->Length;
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
    if (Image->BestHeap)
    {
        oHmbRemove(Image, Image->BestHeap, Image->InitialLength);
        oHmbSet(Image, Image->BestHeap, Image->CurrentLength);
    }
    if (!_BitScanReverse(&Index, Image->BaseBitmap))
        return FALSE;

    _BitScanReverse64(&Index2, Image->SubBmp[Index]);
    PHMBLK Blk = Image->HeapArray[Index2 + (Index << 6)];
    if (Image->BestHeap == Blk)
    {
        return FALSE;
    }
    Image->BestHeap = Blk;
    Image->InitialLength = Index2 + (Index << 6);
    Image->CurrentLength = Image->InitialLength;
    return TRUE;
}

void HMAPI oHmbSet(HMIMAGE *Image, PHMBLK Block, UINT8 Length /*in 16 Byte blocks*/)
{
    _bittestandset(&Image->BaseBitmap, Length >> 6);
    Block->Next = NULL;

    if (_bittestandset64(Image->SubBmp + (Length >> 6), Length & 0x3F))
    {
        PHMBLK baseblk = Image->HeapArray[Length];
        baseblk->PrevOrLast->Next = Block;       // set lastblock.next
        Block->PrevOrLast = baseblk->PrevOrLast; // set block.previous
        baseblk->PrevOrLast = Block;             // set firstblack.last
    }
    else
    {
        Block->PrevOrLast = Block;
        Image->HeapArray[Length] = Block;
    }
}

void HMAPI oHmbRemove(HMIMAGE *Image, PHMBLK Block, UINT8 Length)
{
    if (Block->PrevOrLast == Block)
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
        Block->Next->PrevOrLast = Block->PrevOrLast; // set next with last block
        Image->HeapArray[Length] = Block->Next;      // set arr with next block
    }
    else
    {
        // Middle/Ending Heap
        Block->PrevOrLast->Next = Block->Next; // set previous block with value of next block
        if (Block->Next == NULL)
        { // ending heap
            Image->HeapArray[Length]->PrevOrLast = Block->PrevOrLast;
        }
    }
}