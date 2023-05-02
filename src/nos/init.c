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

    KiInitStandardSubsystems();
    KiDumpPhysicalMemoryEntries(); // To determine memory length

    KiInitBootCpu();

    PROCESS* KernelProcess = KiGetProcessById(0);
    KernelProcess->PageTable = (void*)(__readcr3() & ~0xFFF);


    memset(NosInitData->FrameBuffer.BaseAddress, 0xFF, NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution);
    
    KDebugPrint("Test print %x", 0x128B, 12, 53);
    KeMapPhysicalMemory(
        KernelProcess,
        (void*)0x1000,
        (void*)0x280000000,
        10000000,
        PAGE_USER,
        0
    );
    memset(NosInitData->FrameBuffer.BaseAddress, 0, NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution);

    KDebugPrint("Test2 : %c", 'A');
    // KiDumpPhysicalMemoryEntries();

    // NosInitData->EfiRuntimeServices->ResetSystem(EfiResetCold, 0, 0, NULL);
    for(;;) __halt();
}