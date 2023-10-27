#include <hm.h>
#include <hmdef.h>

#pragma pack(push, 0x10)

typedef struct _HMBLK
{
    UINT64 Addr : 63;
    UINT64 MainBlk : 1;

    UINT64 Length;

    struct _HMBLK *Next;

    struct _HMBLK *PrevOrLast;
} HMBLK, *PHMBLK;

#pragma pack(pop)

typedef struct _HMIMAGE
{
    HMUSERHEADER User;
    // UINT64 UnitLength;  Always 0x10
    PHMBLK BestHeap;

    UINT32 BaseBitmap;
    UINT64 SubBmp[4];
    PHMBLK HeapArray[256];
} HMIMAGE;

PVOID HMAPI oHmbAllocate(
    HMIMAGE *Image,
    UINT64 Length)
{
    if (Image->BestHeap->Length >= Length)
    {
    }
    else
    {
        PHMBLK Blk = oHmbLookup(Image);
        if (Blk->Length >= Length)
        {
        }
    }
}

BOOLEAN HMAPI oHmbFree(HMIMAGE *Image, void *Ptr)
{
}

PHMBLK HMAPI oHmbLookup(HMIMAGE *Image)
{
    UINT64 Index, Index2;
    if (!_BitScanReverse(&Index, Image->BaseBitmap))
        return NULL;
    _BitScanReverse64(&Index2, Image->SubBmp);
    return Image->HeapArray[Index2 + (Index << 6)];
}

void HMAPI oHmbSet(HMIMAGE *Image, PHMBLK Block, UINT8 Length /*in 16 Byte blocks*/)
{
    _bittestandset(&Image->BaseBitmap, Length & 3);
    baseblk->Next = NULL;

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