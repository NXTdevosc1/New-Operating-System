/*
 * This heap allocator permits : Simultanous heap allocation and freeing between multiple processors
 * Extremely fast and easy setup
 * Sharing between two Heap images
 */
#define __CRT_SRC
#include <hmdef.h>
#include <crt.h>
#include <intrin.h>

UINT64 HMAPI HmCreateImage(
    IN OUT HMIMAGE *Image,
    IN UINT Flags,
    OUT UINT64 *Commit,     // Preallocate memory
    IN UINT64 BaseUAddress, // in units
    IN UINT64 EndUAddress,  // in units
    IN UINT64 UnitLength,   // Should be in powers of 2
    IN OPT UINT CallbackMask,
    IN OPT HEAP_MANAGER_CALLBACK Callback)
{

    ObjZeroMemory(Image);

    Image->BaseAddress = BaseUAddress;
    Image->EndAddress = EndUAddress;

    Image->UnitLength = UnitLength;
    Image->CallbackMask = CallbackMask;
    Image->Callback = Callback;
    Image->Flags = Flags;

    Image->TotalSpace = EndUAddress - BaseUAddress;

    UINT64 TotalLength = AlignForward(Image->TotalSpace >> 3, 0x1000);
    if (Flags & HIMAGE_COMMIT_ADDRESSMAP)
    {
        *Commit = TotalLength >> 12;
    }
    else
    {
        Image->LenAddrDir = AlignForward(Image->TotalSpace >> 15, 0x1000);
        *Commit = Image->LenAddrDir >> 12;
        TotalLength += Image->LenAddrDir;
    }

    Image->ReservedAreaLength = TotalLength;

    return TotalLength >> 12;
}

static inline void HMDECL HmUnlink(HMIMAGE *Image, HMHEAP *Heap)
{
    if (Heap->Previous)
    {
        Heap->Previous->Next = Heap->Next;
    }
    if (Heap->Next)
    {
        Heap->Next->Previous = Heap->Previous;
    }
    if (Image->Mem[Heap->Exponent] == Heap)
    {
        Image->Mem[Heap->Exponent] = Heap->Next;
        if (!Heap->Next)
        {
            _bittestandreset64(&Image->PresentMem, Heap->Exponent);
        }
    }
    Heap->Next = NULL;
    Heap->Previous = NULL;
}

static inline void HMDECL HmPlace(HMIMAGE *Image, HMHEAP *Heap)
{
    _BitScanReverse(&Heap->Exponent, Heap->CurrentLength);
    KDebugPrint("Place %x EXP %x", Heap->CurrentLength, Heap->Exponent);
    if (_bittestandset64(&Image->PresentMem, Heap->Exponent))
    {
        Image->MemEnd[Heap->Exponent]->Next = Heap;
        Heap->Previous = Image->MemEnd[Heap->Exponent];
        Image->MemEnd[Heap->Exponent] = Heap;
    }
    else
    {
        Image->Mem[Heap->Exponent] = Heap;
        Image->MemEnd[Heap->Exponent] = Heap;
    }
}

// Sets recent heap
static inline void HMDECL HmLookup(HMIMAGE *Image)
{
    ULONG Index;
    if (!_BitScanReverse(&Index, Image->PresentMem))
    {
        KDebugPrint("BUG 0");
        while (1)
            __halt();
    }
    HMHEAP *Largest = Image->Mem[Index];

    HMHEAP *hp = Largest;
    // TODO : Use more efficient method instead of looping lookup
    while (hp)
    {
        if (hp->CurrentLength > Largest->CurrentLength)
            Largest = hp;

        hp = hp->Next;
    }
    Image->RecentHeap = Largest;
}

void HMAPI HmInitImage(
    HMIMAGE *Image,
    void *ReservedArea,
    UINT64 InitialHeapUAddress,
    UINT InitialHeapULength)
{
    Image->ReservedArea = ReservedArea;
    if (Image->Flags & HIMAGE_COMMIT_ADDRESSMAP)
    {
        Image->AddressMap = Image->ReservedArea;
        XmemsetAligned(Image->AddressMap, 0, Image->ReservedAreaLength >> 7);
    }
    else
    {
        Image->AddressDirectory = Image->ReservedArea;
        Image->AddressMap = (UINT64 *)((char *)Image->ReservedArea + Image->LenAddrDir);
        XmemsetAligned(Image->AddressDirectory, 0, Image->LenAddrDir >> 7);
    }
    Image->InitialHeap.Address = InitialHeapUAddress;
    Image->InitialHeap.FullLength = InitialHeapULength;
    Image->InitialHeap.CurrentLength = InitialHeapULength;
    KDebugPrint("I %x %x", InitialHeapULength, Image->InitialHeap.CurrentLength);
    // Find exponent
    HmPlace(Image, &Image->InitialHeap);
    while (1)
        ;
    HmLookup(Image);

    Image->TotalUnits = InitialHeapULength;
    Image->RecentHeap = &Image->InitialHeap;
}

PVOID HMAPI HmLocalHeapUnsufficientAlloc(IN HMIMAGE *Image, IN UINT64 Size)
{
    HMHEAP *Heap = Image->RecentHeap;
    if (Image->TotalUnits - Image->UsedUnits < Size)
        goto clb;
    // move the heap, find a larger heap
    HmUnlink(Image, Heap);
    HmPlace(Image, Heap);
    HmLookup(Image);
    Heap = Image->RecentHeap;
    if (Heap->CurrentLength < Size)
    {
    clb:
        if (_bittest(&Image->CallbackMask, HmCallbackRequestMemory))
        {
            if (Image->Callback(Image, HmCallbackRequestMemory, Size, (UINT64)&Heap))
            {
                Image->RecentHeap = Heap;
                PVOID p;
                HmLocalAlloc(Image, Size, p);
                return p;
            }
            else
                return NULL;
        }
        else
            return NULL;
    }
    PVOID p;
    HmLocalAlloc(Image, Size, p);
    return p;
}

void HMAPI HmRecentHeapEmpty(IN HMIMAGE *Image)
{
    HMHEAP *Heap = Image->RecentHeap;
    // Exclude
    HmLookup(Image);
    if (Image->RecentHeap != Heap)
    {
        HmUnlink(Image, Heap);
    }

    if (!Heap->FullLength)
    {
        // Unlink subheap
        KDebugPrint("Handle CASE 0");
        while (1)
            __halt();
    }
}

// PVOID HMAPI HmLocalAllocate(
//     IN HMIMAGE *Image,
//     IN UINT64 Size)
// {
// }