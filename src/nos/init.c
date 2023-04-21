/*
 * init.c
 * This file contains the initialization function of the Operating System Kernel
*/
#include <nos/nos.h>
/*
 * The initialization entry point of the NOS Kernel
*/


void __declspec(noreturn) NosSystemInit() {
    // memset(NosInitData->FrameBuffer.BaseAddress, 0xFF, 0x1000);
    SerialLog("NOS_KERNEL : Kernel Booting...");

    char bf[100];
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

    // NosInitData->EfiRuntimeServices->ResetSystem(EfiResetCold, 0, 0, NULL);
    for(;;);
}