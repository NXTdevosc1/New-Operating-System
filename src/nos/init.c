/*
 * init.c
 * This file contains the initialization function of the Operating System Kernel
*/
#include <nos/nos.h>
/*
 * The initialization entry point of the NOS Kernel
*/

char bf[100];

extern void NOSINTERNAL KiDumpPhysicalMemoryEntries();

void __declspec(noreturn) NosSystemInit() {
    SerialLog("NOS_KERNEL : Kernel Booting...");

    
    _itoa(102, bf, 10);
    SerialLog(bf);
    
    SerialLog("PHYS_ADDR, VIRT_ADDR, SIZE");
    _ui64toa((UINT64)NosInitData->NosPhysicalBase, bf, 0x10);
    SerialLog(bf);
    _ui64toa((UINT64)NosInitData->NosKernelImageBase, bf, 0x10);
    SerialLog(bf);
    _ui64toa((UINT64)NosInitData->NosKernelImageSize, bf, 0x10);
    SerialLog(bf);

    KiInitBootCpu();
    KiDumpProcessors();
    KiDumpPhysicalMemoryEntries();
    memset(NosInitData->FrameBuffer.BaseAddress, 0xFF, NosInitData->FrameBuffer.Pitch * NosInitData->FrameBuffer.VerticalResolution);
    // NosInitData->EfiRuntimeServices->ResetSystem(EfiResetCold, 0, 0, NULL);
    for(;;);
}