
/*
 HEAP MANAGER AREA INTERFACE

 Performs optimized sorting to process a heap request in the fastest way possible
*/

#define __VMMSRC
#include <nosdef.h>
#include <intrin.h>
#include <hmdef.h>

typedef struct _hmpll
{
    UINT Level;
    UINT Bitmap68;      // bits 6-8
    UINT64 Bitmap05[8]; // bits 0-5
    VMMHEADER *Headers[512];
    struct
    {
        VMMHEADER *Header;
        UINT16 LenStart;
        UINT16 LenCurrent;
    } Cl; // Current largest
} PAGELEVEL_LENGTHCHAIN;

typedef struct _HMIMAGE
{
    HMUSERHEADER User;
    UINT8 NumLevels;
    UINT64 bLevels;
    PAGELEVEL_LENGTHCHAIN *Mem;

    UINT DescSize;
} HMIMAGE;

void HMAPI VmmCreate(HMIMAGE *Image, UINT8 NumLevels, void *Mem, UINT DescriptorSize)
{
    ObjZeroMemory(Image);
    Image->DescSize = sizeof(VMMHEADER) + AlignForward(DescriptorSize, 0x10);
    Image->NumLevels = NumLevels;
    Image->Mem = Mem;
    Image->User.Buffer = Mem;
    for (int i = 0; i < NumLevels; i++)
    {
        ObjZeroMemory(Image->Mem + i);
        Image->Mem[i].Level = i;
    }
}

inline void HMAPI VmmInsert(PAGELEVEL_LENGTHCHAIN *Chain,
                            VMMHEADER *Desc,
                            UINT16 Length)
{

    Desc->Next = NULL;
    Chain->Bitmap68 |= (1 << (Length >> 6));
    if (_bittestandset64(Chain->Bitmap05 + (Length >> 6), Length & 0x3F))
    {
        Chain->Headers[Length]->LastOrPrev->Next = Desc;
        Desc->LastOrPrev = Chain->Headers[Length]->LastOrPrev;
        Chain->Headers[Length]->LastOrPrev = Desc;
    }
    else
    {
        Chain->Headers[Length] = Desc;
        Desc->LastOrPrev = Desc;
    }
}

inline void HMAPI VmmRemove(PAGELEVEL_LENGTHCHAIN *Chain,
                            VMMHEADER *Desc,
                            UINT16 Length)
{

    if (Desc->LastOrPrev == Desc)
    {
        // Only descriptor
        _bittestandreset64(Chain->Bitmap05 + (Length >> 6), Length & 0x3F);
        if (!Chain->Bitmap05[Length >> 6])
        {
            Chain->Bitmap68 &= ~(1 << (Length >> 6));
        }
        Chain->Headers[Length] = NULL;
    }
    else if (Chain->Headers[Length] == Desc)
    {
        // First descriptor
        Desc->Next->LastOrPrev = Desc->LastOrPrev;
        Chain->Headers[Length] = Desc->Next;
    }
    else
    {
        // Middle or ending descriptor
        Desc->LastOrPrev->Next = Desc->Next;
        if (Desc->Next == NULL)
        {
            // ending descriptor
            Chain->Headers[Length]->LastOrPrev = Desc->LastOrPrev;
        }
    }
}

static inline BOOLEAN _VmmInstantLookup(PAGELEVEL_LENGTHCHAIN *Chain)
{
    if (Chain->Cl.Header)
    {
        VmmRemove(Chain, Chain->Cl.Header, Chain->Cl.LenStart);
        VmmInsert(Chain, Chain->Cl.Header, Chain->Cl.LenCurrent);
    }

    ULONG Index, Index2;

    if (!_BitScanReverse(&Index, Chain->Bitmap68))
    {
        Chain->Cl.LenStart = Chain->Cl.LenCurrent;
        return FALSE;
    }

    _BitScanReverse64(&Index2, Chain->Bitmap05[Index]);

    Chain->Cl.LenStart = Index << 6 | (Index2);
    Chain->Cl.LenCurrent = Chain->Cl.LenStart;
    Chain->Cl.Header = Chain->Headers[Chain->Cl.LenStart];

    return TRUE;
}

// Each higher memory allocation maps a bitmap (2bit based bitmap)

PVOID HMAPI VmmAllocate(HMIMAGE *Image, UINT Level, UINT16 Count)
{
    PAGELEVEL_LENGTHCHAIN *Ch = Image->Mem + Level;
    if (Ch->Cl.LenCurrent >= Count || (_VmmInstantLookup(Ch) && Ch->Cl.LenCurrent >= Count))
    {
        PVOID Ret = (PVOID)(Ch->Cl.Header->Address << 12);
        Ch->Cl.Header->Address += (Count << (9 * Level));
        Ch->Cl.LenCurrent -= Count;
        return Ret;
    }
    else
        return NULL;
}

// MANAGED BY the parent allocator (depends wheter physical allocator, or virtual address allocator)
// void VmmFree(HMIMAGE *Image, PVOID Ptr, UINT64 Length)
// {
//     // TODO : Resolve ptr into levels and set correct value
// }
