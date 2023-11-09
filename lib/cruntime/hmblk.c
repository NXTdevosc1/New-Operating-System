#include <hmdef.h>
#include <mm.h>

typedef struct _HMIMAGE
{
    HMUSERHEADER User;
    // UINT64 UnitLength;  Always 0x10
    HMALLOCATE_ROUTINE SrcAlloc;
    HMFREE_ROUTINE SrcFree;
    PHMBLK BestHeap;
    UINT64 InitialLength;
    UINT64 CurrentLength;

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
    KDebugPrint("alloc");
    Length += 2; // add 32 byte header
    UINT64 NumPages = Length >> 8;
    if (NumPages)
    { // exceeds 1 page
        if (Length & 0xFF)
        {
        NewBlockAlloc:
            KDebugPrint("case2");
            NumPages++;
            Length += 2; // another descriptor
            if (Length > (NumPages << 8))
            {
                KDebugPrint("test case 1");
                NumPages++;
            }
            UINT64 RemainingSize = (NumPages << 8) - Length;
            PVOID Mem = Image->SrcAlloc(NumPages);
            KDebugPrint("Remaining size %d bytes", RemainingSize << 4);
            if (!Mem)
                return NULL;

            PHMBLK Desc0 = Mem, Desc1 = (PHMBLK)((char *)Mem + ((Length - 2) << 4));
            KDebugPrint("Desc0 %x Desc1 %x Max %x", Desc0, Desc1, (char *)Mem + (NumPages << 12));
            Desc0->Addr = (UINT64)(Desc0 + 1);
            Desc0->MainBlk = 1;
            Desc1->Addr = (UINT64)(Desc1 + 1);
            oHmbSet(Image, Desc1, RemainingSize);
            return Desc0 + 1;
        }
        PHMBLK Desc = Image->SrcAlloc(NumPages);
        Desc->Addr = (UINT64)(Desc + 1);
        Desc->MainBlk = 1;
        return Desc + 1;
    }
    else
    {
        KDebugPrint("alloc2");

        if (Image->CurrentLength < Length && (!oHmbLookup(Image) || Image->CurrentLength < Length))
        {
            KDebugPrint("alloc3");

            goto NewBlockAlloc;
        }
        KDebugPrint("alloc4");

        PVOID Mem = (PVOID)(Image->BestHeap->Addr << 4);
        Image->BestHeap->Addr += Length;
        Image->CurrentLength -= Length;
        return Mem;
    }
}

BOOLEAN HMAPI oHmbFree(HMIMAGE *Image, void *Ptr)
{
    return FALSE;
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

    _BitScanReverse64(&Index2, Image->SubBmp + Index);
    PHMBLK Blk = Image->HeapArray[Index2 + (Index << 6)];

    if (Image->BestHeap == Blk)
    {
        KDebugPrint("ret false");
        return FALSE;
    }
    KDebugPrint("ret true");
    Image->BestHeap = Blk;
    Image->InitialLength = Index2 + (Index << 6);
    Image->CurrentLength = Image->InitialLength;
    return TRUE;
}

void HMAPI oHmbSet(HMIMAGE *Image, PHMBLK Block, UINT8 Length /*in 16 Byte blocks*/)
{
    _bittestandset(&Image->BaseBitmap, Length & 3);
    Block->Next = NULL;

    if (_bittestandset64(Image->SubBmp + (Length & 3), Length >> 2))
    {
        PHMBLK baseblk = Image->HeapArray[Length];
        baseblk->PrevOrLast->Next = Block;
        Block->PrevOrLast = baseblk->PrevOrLast;
        baseblk->PrevOrLast = Block;
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
        _bittestandreset64(Image->SubBmp + (Length & 3), Length >> 2);
        if (!Image->SubBmp[Length & 3])
        {
            _bittestandreset(&Image->BaseBitmap, Length & 3);
        }
        Image->HeapArray[Length] = NULL;
    }
    else if (Image->HeapArray[Length] == Block)
    {
        // Starting heap
        Block->Next->PrevOrLast = Block->PrevOrLast;
        Image->HeapArray[Length] = Block->Next;
    }
    else
    {
        // Middle/Ending Heap
        Block->PrevOrLast->Next = Block->Next;
    }
}