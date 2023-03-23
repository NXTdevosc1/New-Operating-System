/*
 * init.c
 * This file contains the initialization function of the Operating System Kernel
*/
#define DEBUG 1
#include <nos.h>
#include <processor/processor.h>
#include <lock/lock.h>
/*
 * The initialization entry point of the NOS Kernel
*/
void __declspec(noreturn) NosSystemInit() {
    SerialLog("NOS_KERNEL : Kernel Booting...");
    char bn[100];
    CpuReadBrandName(bn);
    SerialLog(bn);
    for(UINT32 i = 0;i<0x10000;i++) {
        ((UINT32*)NosInitData->FrameBuffer.BaseAddress)[i] = 0xff;
    }
    MUTEX mt;
    KeInitMutex(&mt);
    KeMutexEnter(NULL, &mt, 0);
    KeMutexRelease((void*)NULL, &mt);
    KeMutexEnter((void*)1, &mt, 0);

    for(UINT32 i = 0;i<0x10000;i++) {
        ((UINT32*)NosInitData->FrameBuffer.BaseAddress)[i] = 0xff00;
    }

    // NosInitData->EfiRuntimeServices->ResetSystem(EfiResetCold, 0, 0, NULL);
    for(;;);
}