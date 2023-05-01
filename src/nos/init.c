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

    KiInitStandardSubsystems();
    
    KiDumpPhysicalMemoryEntries();

    KiDumpProcessors();
    KiInitBootCpu();

    void *p = NULL;
    SerialLog("Testing Memory Allocations");
    memset(NosInitData->FrameBuffer.BaseAddress, 0x20, NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution);
        float z = 0;
    for(int c = 0;c<1000000;c++) {
        KeAllocatePhysicalMemory(0, 1, &p);
        // _ui64toa((UINT64)p, bf, 0x10);
        // SerialLog(bf);
    }

    SerialLog("finish");
    PROCESS* KernelProcess = KiGetProcessById(0);
    _ui64toa((UINT64)KiGetProcessById(1), bf, 0x10);
    SerialLog(bf);

    

    _ui64toa((UINT64)KiGetThreadById(0), bf, 0x10);
    SerialLog(bf);
    KeCreateThread(KernelProcess, NULL, 0, (void*)0x1923);
    _ui64toa((UINT64)KiGetThreadById(0), bf, 0x10);
    SerialLog(bf);

    memset(NosInitData->FrameBuffer.BaseAddress, 0xFF, NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution);
    
    
    // KiDumpPhysicalMemoryEntries();

    // NosInitData->EfiRuntimeServices->ResetSystem(EfiResetCold, 0, 0, NULL);
    for(;;) __halt();
}