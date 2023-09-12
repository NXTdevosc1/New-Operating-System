/*
 * This heap allocator permits : Simultanous heap allocation and freeing between multiple processors
 * Extremely fast and easy setup
 * Sharing between two Heap images
 */

#include <hmdef.h>

// UINT64 HMAPI HmCreateImage(
//     OUT HMIMAGE *Image,
//     UINT64 BaseAddress,
//     UINT64 EndAddress,
//     UINT64 UnitLength,         // Should be in powers of 2
//     BOOLEAN EnableBlockHeader, // Unit length is ignored abd set to 32 bytes
//     UINT64 CallbackMask,
//     HEAP_MANAGER_CALLBACK Callback)
// {
//     ObjZeroMemory(Image);
//     Image->BaseAddress = BaseAddress;
//     Image->EndAddress = EndAddress;

//     UINT64 TotalSpace = EndAddress - BaseAddress;
//     if (TotalSpace > ((UINT)-1))
//     {
//         Image->SizeLevels = 3; // 8 bits per level + last level
//     }
//     else
//     {
//         _BitScanReverse64(&Image->SizeLevels, )
//     }
// }

// HMHEAP *HmFetchLargestHeap(
//     HMIMAGE *Image,
//     UINT Level,
//     UINT Index,
//     UINT64 *Map)
// {

//     if (!RemainingLevels)
//     {
//     }
//     else
//     {
//         UINT64 *Val = Map + (Level << 3);
//         ULONG idx;
//         _BitScanReverse64(&idx, *CurrentBmp);
//     }
// }