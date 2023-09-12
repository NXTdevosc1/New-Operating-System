#include <mm.h>

// BOOLEAN KRNLAPI HwMapMemory(
//     UINT64 *PageTable,
//     void *PhysicalStart,
//     void *VirtualStart,
//     UINT64 Count,
//     UINT Attributes)
// {
//     UINT PageSize = 0x1000;
//     if (Attributes & (MEM_2MB))
//     {
//         PageSize *= 0x200;
//     }
//     else if (Attributes & (MEM_1GB))
//     {
//         PageSize *= (0x200 * 0x200);
//     }

//     // PML4

//     // PDP

//     // PD

//     // PT
// }

// PVOID KRNLAPI MAllocateVirtualMemory(
//     void *VirtualStart,
//     UINT64 Count,
//     UINT Attributes)
// {
// }
6