/*
 * This heap allocator permits : Simultanous heap allocation and freeing between multiple processors
 * Extremely fast and easy setup
 * Sharing between two Heap images
*/

#include <hmdef.h>



char _format[0x200];

static inline UINT64 _pow(UINT64 a, UINT64 b) {
    if(b == 0) return 1;
    UINT64 Ret = a;
    b--;
    while(b) {
        Ret = Ret * a;
        b--;
    }
    return Ret;
}

UINT64 HMAPI HmCreateImage(
    HMIMAGE* Image,
    UINT64 UnitLength,
    UINT64 MaxLength, // Max heap length in units
    HMIMAGE* AllocateFrom,
    BOOLEAN InsertBlockHeader
) {

    ObjZeroMemory(Image);
    _BitScanReverse64(&Image->LastBit, MaxLength);
    if(Image->LastBit < 12) return 0;

    

    Image->UnitLength = UnitLength;
    Image->AllocateFrom = AllocateFrom;
    Image->MaxLength = MaxLength;
    Image->InsertBlockHeader = InsertBlockHeader;



    Image->SubLevelEntries = ((Image->LastBit - 6) / 3);
    Image->FirstLevelEntries = 1 << (Image->SubLevelEntries + ((Image->LastBit - 6) % 3));
    Image->SubLevelEntries = 1 << Image->SubLevelEntries;

    Image->FirstLevelLength = sizeof(HMMAP) + AlignForward(Image->FirstLevelEntries, 64) / 8;
    Image->SubLevelLength = sizeof(HMMAP) + AlignForward(Image->SubLevelEntries, 64) / 8;

    UINT64 TotalAddressSpace = Image->FirstLevelLength;
    TotalAddressSpace += ((UINT64)Image->FirstLevelEntries * Image->SubLevelLength);
    TotalAddressSpace += ((UINT64)Image->FirstLevelEntries * Image->SubLevelEntries * Image->SubLevelLength);
    TotalAddressSpace += ((UINT64)Image->FirstLevelEntries * Image->SubLevelEntries * Image->SubLevelEntries * HM_LAST_TREE_LENGTH);

    Log("FLE %d SLE %d", Image->FirstLevelEntries, Image->SubLevelEntries);
    Log("FLL %d SLL %d", Image->FirstLevelLength, Image->SubLevelLength);


    KDebugPrint("Total address space %u Bytes 0-%x", TotalAddressSpace, TotalAddressSpace);
    return TotalAddressSpace;
}

void HMAPI HmInitImage(
    HMIMAGE* Image,
    void* AddressSpace, // No need to be mapped
    void* InitialHeapAddress,
    UINT64 uInitialHeapLength,
    UINT64 CallbackMask,
    HEAP_MANAGER_CALLBACK Callback
) {
    Image->SizeMapStart = AddressSpace;
    Image->InitialHeap.Address = (UINT64)InitialHeapAddress >> Image->UnitLengthShift;
    Image->InitialHeap.Length = uInitialHeapLength;
    Image->CallbackMask = CallbackMask;
    Image->Callback;
}

static __declspec(align(0x10)) UINT32 BmpShift[4] = {6, 6, 6, 6};
static __declspec(align(0x10)) UINT32 SubBmpMask[4] = {0x3F, 0x3F, 0x3F, 0x3F};
void HMAPI HmMapHeap(
    HMIMAGE* Image,
    void* Space,
    UINT64 Key
) {
    __declspec(align(0x10)) UINT32 Lvl[4];
    Lvl[0] = (Key >> Image->FirstLvlBitshift) & Image->FirstLvlBitmask;
    Lvl[1] = (Key >> Image->SecondLvlBitshift) & Image->SubLvlBitmask;
    Lvl[2] = (Key >> Image->ThirdLvlBitshift) & Image->SubLvlBitmask;
    Lvl[3] = (Key) & 0x3F;
    __declspec(align(0x10)) UINT32 Bmp[4];
    *(__m128i*)Bmp = _mm_srl_epi32(*(__m128i*)Lvl, *(__m128i*)&BmpShift);
    UINT32 SubBmp[4] = {Lvl[0] & 0x3F, Lvl[1] & 0x3F, };
    *(__m128*)SubBmp = _mm_and_ps(*(__m128*)Lvl, *(__m128*)&SubBmpMask);


}