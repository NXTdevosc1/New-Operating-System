#include <hmdef.h>
#include <mm.h>

typedef struct _HMIMAGE
{
    HMUSERHEADER User;
    // UINT64 UnitLength;  Always 0x10
    PHMBLK BestHeap;
    UINT64 InitialLength;
    UINT64 CurrentLength;

    UINT32 BaseBitmap;
    UINT64 SubBmp[4];
    PHMBLK HeapArray[256];
} HMIMAGE;

PVOID HMAPI oHmbAllocate(
    HMIMAGE *Image,
    UINT64 Length)
{
    Length += 2; // add 32 byte header
    if (Image->CurrentLength < Length && (!oHmbLookup(Image) || Image->CurrentLength < Length))
    {
        KDebugPrint("OHMB : case 0");
        while (1)
            __halt();
    }
    PVOID ret = (PVOID)(Image->BestHeap->Addr << 4);
    Image->BestHeap->Addr += Length;
    Image->CurrentLength -= Length;
    return ret;
}

BOOLEAN HMAPI oHmbFree(HMIMAGE *Image, void *Ptr)
{
    return FALSE;
}

BOOLEAN HMAPI oHmbLookup(HMIMAGE *Image)
{
    UINT64 Index, Index2;
    if (!_BitScanReverse(&Index, Image->BaseBitmap))
        return FALSE;
    _BitScanReverse64(&Index2, Image->SubBmp);
    PHMBLK Blk = Image->HeapArray[Index2 + (Index << 6)];
    if (Image->BestHeap == Blk)
        return FALSE;

    if (Image->BestHeap)
    {
        oHmbRemove(Image, Image->BestHeap, Image->InitialLength);
        oHmbSet(Image, Image->BestHeap, Image->CurrentLength);
    }
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