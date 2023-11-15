#include <nos/nos.h>
#include <mm.h>
#include <nos/mm/physmem.h>

PVOID KRNLAPI MmRequestContiguousPagesNoDesc(
    IN HMIMAGE *Image,
    IN UINT64 Length)
{
    // TODO : In case length > Max pages to request (511)

    if (Image->User.BestHeap.RemainingLength < Length && (!oHmpLookup(Image) || Image->User.BestHeap.RemainingLength < Length))
    {
        return NULL;
    }

    PVOID ptr = (PVOID)Image->User.BestHeap.Block->Address;
    Image->User.BestHeap.Block->Address += Length << Image->User.AlignShift;
    Image->User.BestHeap.RemainingLength -= Length;
    return ptr;
}

static inline PVOID MmiIncludeAdditionalPages(
    HMIMAGE *Image,
    UINT64 Length)
{
    if (!Image->User.HigherImage)
        return NULL;

    UINT64 Rem = ExcessBytes(Length, 0x200);
    UINT64 NLength = AlignForward(Length, 0x200) >> 9;
    PVOID p = MmRequestContiguousPagesNoDesc(Image->User.HigherImage, NLength);
    if (!p)
        return MmiIncludeAdditionalPages(Image->User.HigherImage, NLength);
    else if (Rem)
    {
        // set free block
        KPAGEHEADER *Hdr = MmAllocatePool(sizeof(KPAGEHEADER), 0);
        if (!Hdr)
        {
            KDebugPrint("HDR Alloc pool failed, freeing memory...");
            MmFreePages(p);
            return NULL;
        }

        Hdr->Header.Address = (UINT64)p + ((Length) << Image->User.AlignShift);
        // KDebugPrint("Rem addr %x calculated rem %x Rem %x", Hdr->Header.Address, (NLength << 9) - Length, Rem);
        oHmpSet(Image, &Hdr->Header, Rem);
    }
    return p;
}

PVOID KRNLAPI MmRequestContiguousPages(
    UINT PageSize,
    UINT64 Length)
{
    HMIMAGE *Image = _NosHeapImages + PageSize;
    PVOID p = MmRequestContiguousPagesNoDesc(Image, Length);
    if (!p)
        return MmiIncludeAdditionalPages(Image, Length);

    return p;
}

void KRNLAPI MmFreePages(
    PVOID Address)
{
}
