
/*
 HEAP MANAGER AREA INTERFACE

 Performs optimized sorting to process a heap request in the fastest way possible
*/

#include <nosdef.h>
#include <intrin.h>
#include <hmdef.h>

typedef struct _hmphdr
{
    UINT64 LastDescriptor : 1;
    UINT64 Attributes : 11;
    UINT64 Address : 52;
    struct _hmphdr *Next;
    struct _hmphdr *LastOrPrev;
} PAGEHEADER;

typedef struct
{
    UINT Bitmap67;      // bits 6-7
    UINT64 Bitmap05[4]; // bits 0-5
    PAGEHEADER Headers[512];
} PAGELEVEL_LENGTHCHAIN;

typedef struct _HMIMAGE
{
    HMUSERHEADER User;
    UINT8 NumLevels;
    UINT64 bLevels;
    PAGELEVEL_LENGTHCHAIN *Mem;

    struct
    {
        PAGEHEADER *Header;
        UINT64 LenStart;
        UINT64 LenCurrent;
        UINT Level;
    } Cl; // Current largest

    UINT DescSize;
} HMIMAGE;

void HmPageImageCreate(HMIMAGE *Image, UINT8 NumLevels, UINT64 *Mem, UINT DescriptorSize)
{
    ObjZeroMemory(Image);
    Image->DescSize = sizeof(PAGEHEADER) + AlignForward(DescriptorSize, 0x10);
    Image->NumLevels = NumLevels;
    Image->Mem = Mem;
}

void HmPageImageInsert(HMIMAGE *Image, PVOID Descriptor, UINT Level, UINT16 Length)
{

    PAGEHEADER *Desc = Descriptor;
    Desc->Next = NULL;
    PAGELEVEL_LENGTHCHAIN *Chain = Image->Mem + Level;
    Chain->Bitmap67 |= (1 << (Length >> 6));
    if (_bittestandset64(Chain->Bitmap05 + (Length >> 6), Length & 0x3F))
    {
        Chain->Headers[Length].LastOrPrev->Next = Desc;
        Desc->LastOrPrev = Chain->Headers[Length].LastOrPrev;
        Chain->Headers[Length].LastOrPrev = Desc;
    }
    else
    {
        Chain->Headers[Length] = Desc;
        Desc.LastOrPrev = Desc;
    }

    Image->bLevels |= 1 << Level;
}

void HmPageImageRemove(HMIMAGE *Image, PVOID Descriptor, UINT Level, UINT64 Length)
{
    PAGEHEADER *Desc = Descriptor;
    PAGELEVEL_LENGTHCHAIN *Chain = Image->Mem + Level;

    if (Desc->LastOrPrev == Desc)
    {
        // Only descriptor
        _bittestandreset64(Chain->Bitmap05 + (Length >> 6), Length & 0x3F);
        if (!Chain->Bitmap05[Length >> 6])
        {
            Chain->Bitmap67 &= ~(1 << (Length >> 6));
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
            Chain->Headers[Length].LastOrPrev = Desc->LastOrPrev;
        }
    }

    if (!Chain->Bitmap67)
        Image->bLevels &= ~(1 << Level);
}

static inline BOOLEAN _HmiInstantLookup(HMIMAGE *Image)
{

    if (Image->Cl.Header)
    {
        HmPageImageRemove(Image, Image->Cl.Header, Image->Cl.Level, Image->Cl.LenStart);
        HmPageImageInsert(Image, Image->Cl.Header, Image->Cl.Level, Image->Cl.LenCurrent);
    }
    ULONG Level, Index, Index2;
    if (!_BitScanReverse64(&Level, Image->bLevels))
    {
        Image->Cl.LenStart = Image->Cl.LenCurrent;
        return FALSE;
    }

    PAGELEVEL_LENGTHCHAIN *Chain = Image->Mem + Level;
    _BitScanReverse(&Index, Chain->Bitmap67);
    _BitScanReverse64(&Index2, Chain->Bitmap05[Index]);

    Image->Cl.LenStart = Index << 6 | (Index2);
    Image->Cl.LenCurrent = Image->Cl.LenStart << (Level * 9);
    Image->Cl.Header = Chain->Headers[Image->Cl.LenStart];
    Image->Cl.Level = Level;

    return TRUE;
}

PVOID HmPageImageAllocateContiguous(HMIMAGE *Image, UINT64 Length)
{
    if (Image->Cl.LenCurrent >= Length || (_HmiInstantLookup(Image) && Image->Cl.LenCurrent >= Length))
    {
        PVOID Ret = (Image->Cl.Header->Address << 12);
    }
    else
        return NULL;
}

PVOID HmPageImageAllocateFragmented(HMIMAGE *Image, UINT64 Length)
{
}

BOOLEAN HmPageImageFreeFull(HMIMAGE *Image, PVOID Ptr)
{
}

BOOLEAN HmPageImageFreePartial(HMIMAGE *Image, PVOID Ptr, UINT64 Length)
{
}
