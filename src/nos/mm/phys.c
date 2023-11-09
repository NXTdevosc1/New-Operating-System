#include <nos/nos.h>
#include <mm.h>
#include <nos/mm/physmem.h>

PVOID KRNLAPI MmRequestContiguousPagesNoDesc(
    IN UINT PageSize,
    IN OUT UINT64 *LengthRemaining,
    PVOID *SystemReserved)
{
    UINT64 Length = *LengthRemaining;
    HMIMAGE *Image = _NosPhysical4KBImage;
    // TODO : In case length > Max pages to request (511)
    if (PageSize == LargePageSize)
        Image = _NosPhysical2MBImage;
    else if (PageSize == HugePageSize)
        Image = _NosPhysical1GBImage;
    else if (PageSize >= MaxPageSize)
        KeRaiseException(STATUS_INVALID_PARAMETER);

    if (Image->User.BestHeap.RemainingLength < Length && (!oHmpLookup(Image) || Image->User.BestHeap.RemainingLength < Length))
    {

        if (!Image->User.HigherImage)
            return NULL;

        UINT PortionLength = Length > 0x1FF ? (Length >> 9) : 1;
        PVOID p = MmRequestContiguousPages(PageSize + 1, PortionLength);
        if (!p)
            return NULL;

        *LengthRemaining = (PortionLength << 9) - Length;
        *SystemReserved = Image;
        KDebugPrint("Higher portion : %x Remaining %x Pages %x", p, (char *)p + (Length << Image->User.AlignShift), *LengthRemaining);
        return p;
    }

    PVOID ptr = (PVOID)Image->User.BestHeap.Block->Address;
    Image->User.BestHeap.Block->Address += Length << Image->User.AlignShift;
    Image->User.BestHeap.RemainingLength -= Length;
    *LengthRemaining = 0;
    return ptr;
}

PVOID KRNLAPI MmRequestContiguousPages(
    UINT PageSize,
    UINT64 Length)
{
    HMIMAGE *Image;
    PVOID p = MmRequestContiguousPagesNoDesc(PageSize, &Length, (void **)&Image);
    if (!p)
        return NULL;
    if (Length)
    {
        // Memory remaining
        KPAGEHEADER *Header = MmAllocatePool(sizeof(KPAGEHEADER), 0);
        if (!Header)
        {
            KeRaiseException(STATUS_OUT_OF_MEMORY);
        }
        Header->Header.Address = (UINT64)p + (Length << Image->User.AlignShift);
        oHmpSet(Image, &Header->Header, Length);
    }
    return p;
}

void KRNLAPI MmFreePages(
    PVOID Address)
{
}
