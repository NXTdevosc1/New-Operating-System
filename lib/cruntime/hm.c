/*
 * This heap allocator permits : Simultanous heap allocation and freeing between multiple processors
 * Extremely fast and easy setup
 * Sharing between two Heap images
*/

#include <hmdef.h>

/*
 * Local functions
 * Either use them in a single thread
 * Or use them in multiple threads with only 1 processor, and disable interrupts before calling each function
*/
// PVOID HMAPI HmLocalAllocate(
//     IN HMIMAGE* Image,
//     IN UINT64 Size
// ) {
//     if(Image->RecentHeap->Length < Size) {
//         // Remap the recent heap now instead of each call [OPTIMIZATION]
//         _HmRemapDescriptor(Image, Image->RecentHeap);
//         _HmSetLargestHeap(Image);
//         if(Image->RecentHeap->Length < Size && !_HmRequestMemory(Image, Size)) return NULL; 
//     }
//     PVOID ret = _HmCreateDescriptor(Image, Image->RecentHeap, Size);

//     Image->UsedUnits+=Size;

//     return ret;
// }

// BOOLEAN HMAPI HmLocalFree(
//     IN HMIMAGE* Image,
//     IN void* Address
// ) {
//     HMHEAP* Heap = _HmFindUnmapDescriptor(Image, Address);
//     if(!Heap) return FALSE;
//     _HmMapDescriptor(Image, Heap);
//     Image->UsedUnits-=Heap->Length;
//     return TRUE;
// }


char _format[0x200];


BOOLEAN HMAPI HmCreateImage(
    IN HMIMAGE* Image,
    IN HMIMAGE* AllocateFrom, // if Set to NULL, descriptors will be allocated within the heap
    IN ULONG UnitLength,
    IN void* StartAddress, // In Bytes, aligned with unit length
    IN void* EndAddress, // In Bytes, aligned with unit length
    IN void* InitialHeapAddress, // In Bytes, aligned with units
    IN UINT64 InitialHeapLength, // In Units
    IN OPT HEAP_MANAGER_CALLBACK Callback
) {
    ObjZeroMemory(Image);
    Image->TotalUnits = InitialHeapLength;
    Image->UnitLength = UnitLength;
    _BitScanReverse64(&Image->LengthShift, UnitLength);
    Image->AllocateFrom = AllocateFrom;
    Image->HeapEventCallback = Callback;
    
    if(AllocateFrom) {
        Image->DescriptorSize = sizeof(HMHEAP) / AllocateFrom->UnitLength;
        if(sizeof(HMHEAP) % AllocateFrom->UnitLength) {
            Image->DescriptorSize++;
        }
    } else {
        Image->DescriptorSize = sizeof(HMHEAP) / Image->UnitLength;
        if(sizeof(HMHEAP) % Image->UnitLength) {
            Image->DescriptorSize++;
        }
    }

    Image->StartAddress = (UINT64)StartAddress / UnitLength;
    Image->EndAddress = (UINT64)EndAddress / UnitLength;
    Image->AddressSpaceLength = Image->EndAddress - Image->StartAddress;

    ULONG LastBit;
    _BitScanReverse64(&LastBit, Image->AddressSpaceLength);

    Log("Desc size %d ADDRSPACE %x Bits %d", Image->DescriptorSize, Image->AddressSpaceLength, LastBit);

    Image->SubLevelEntries = (1 << (LastBit >> 2));
    Image->FirstLevelEntries = (1 << ((LastBit >> 2) + (LastBit & 3)));

    // Calculate first level length, aligned to 64 Bytes (for AVX512)
    UINT64 Len = AlignForward(0x10 + ((Image->FirstLevelEntries >> 7)) + (Image->FirstLevelEntries << 3), 64);
    Len = AlignForward(Len, UnitLength);
    Image->FirstLevelLength = Len >> Image->LengthShift;
    // Calculate sub level length
    Len = AlignForward(0x10 + ((Image->SubLevelEntries >> 7)) + (Image->SubLevelEntries << 3), 64);
    Len = AlignForward(Len, UnitLength);
    Image->SubLevelLength = Len >> Image->LengthShift;

    Log("Required sublevel %d entries, first level : %d entries", Image->SubLevelEntries, Image->FirstLevelEntries);
    Log("First lvl len %u units, Sub lvl len %u units", Image->FirstLevelLength, Image->SubLevelLength);
    Image->InitialHeap.Address = (UINT64)InitialHeapAddress >> Image->LengthShift;
    Image->InitialHeap.Length = InitialHeapLength;

    Image->RecentHeap = &Image->InitialHeap;


    // Bitshifts, masks
    _BitScanReverse(&Image->ThirdLvlBitshift, Image->SubLevelEntries);
    Image->SecondLvlBitshift = Image->ThirdLvlBitshift << 1;
    Image->FirstLvlBitshift = Image->ThirdLvlBitshift << 2;

    Image->SubLvlBitmask = Image->SubLevelEntries - 1;
    Image->FirstLvlBitmask = Image->FirstLevelEntries - 1;

    __m256i* _x = (__m256i*)&Image->Xlvlbitmask;
    _x->m256i_u64[0] = Image->FirstLvlBitmask;
    _x->m256i_u64[1] = Image->SubLvlBitmask;
    _x->m256i_u64[2] = Image->SubLvlBitmask;
    _x->m256i_u64[3] = Image->SubLvlBitmask;

    Image->Xlvlbitshift.m256i_u64[0] = Image->FirstLvlBitshift;
    Image->Xlvlbitshift.m256i_u64[1] = Image->SecondLvlBitshift;
    Image->Xlvlbitshift.m256i_u64[2] = Image->ThirdLvlBitshift;
    Image->Xlvlbitshift.m256i_u64[3] = 0;

    Log("BMSK %x %x %x %x", _x->m256i_u64[0], _x->m256i_u64[1], _x->m256i_u64[2], _x->m256i_u64[3]);
    Log("BSHFT %x %x %x %x", Image->Xlvlbitshift.m256i_u64[0], Image->Xlvlbitshift.m256i_u64[1], Image->Xlvlbitshift.m256i_u64[2], Image->Xlvlbitshift.m256i_u64[3]);


    // Heap trees

    Image->AddressMap = _HmTakeSpace(Image, Image->FirstLevelLength);
    Image->SizeMap = _HmTakeSpace(Image, Image->FirstLevelLength);
    if(!Image->AddressMap || !Image->SizeMap) return FALSE;
    _HmClearMem(Image->AddressMap, Image->FirstLevelLength << Image->LengthShift);
    _HmClearMem(Image->SizeMap, Image->FirstLevelLength << Image->LengthShift);


    Log("ADDR_MAP %x SIZE_MAP %x", Image->AddressMap, Image->SizeMap);

    _HmPutHeap(Image, &Image->InitialHeap, Image->SizeMap, Image->InitialHeap.Length);
    return TRUE;
}


void HMAPI HmLocalCreateHeap(
    IN HMIMAGE* Image,
    IN void* Address,
    IN UINT64 Length
) {

}

HMINTERNAL void* HMDECL _HmTakeSpace(HMIMAGE* Image, UINT64 Units) {
    if(Image->RecentHeap->Length < Units) {
        Log("ATTENTION 1");
        while(1);
    }

    void* ret = (void*)(Image->RecentHeap->Address << Image->LengthShift);

    Image->RecentHeap->Length-=Units;
    Image->RecentHeap->Address+=Units;

    return ret;
}


HMINTERNAL void HMDECL _HmClearMem(void* Mem, UINT64 LengthInBytes) {
    // Current support, SSE
    LengthInBytes>>=4;
    _Memset128A_32(Mem, 0, LengthInBytes);
}

HMINTERNAL void HMDECL _HmPutHeap(
    HMIMAGE* Image,
    HMHEAP* Heap,
    void* Map,
    UINT64 Key
) {
    // Sort the key into tree indexes using AVX
    __m128i _tmp;
    _tmp.m128i_u64[0] = Key;
    __m256i Xlvls = _mm256_broadcastq_epi64(_tmp);
    Xlvls = _mm256_srlv_epi64(Xlvls, Image->Xlvlbitshift);
    *((__m256d*)&Xlvls) = _mm256_and_pd(*(__m256d*)&Xlvls, Image->Xlvlbitmask);

    Log("KEY %x LVL1 %x LVL2 %x LVL3 %x LVL4 %x",
    Key, Xlvls.m256i_u64[0], Xlvls.m256i_u64[1],
    Xlvls.m256i_u64[2], Xlvls.m256i_u64[3]
    );

    // Now allocate the heap in the map
    

    // LVL0
    HMTREEHEADER* Hdr = Map;
    if(Xlvls.m256i_u64[0] < Hdr->Least) Hdr->Least = Xlvls.m256i_u64[0];
    if(Xlvls.m256i_u64[0] > Hdr->Last) Hdr->Last = Xlvls.m256i_u64[0];

    // Hdr->Bitmap[Xlvls.m256i_u64]
    
}