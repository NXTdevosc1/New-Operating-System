/*
 * init.c
 * This file contains the initialization function of the Operating System Kernel
*/

#include <nos.h>

/*
 * The initialization entry point of the NOS Kernel
*/
void __declspec(noreturn) NosSystemInit() {
    for(UINT32 i = 0;i<0x10000;i++) {
        ((UINT32*)NosInitData->FrameBuffer.BaseAddress)[i] = 0xff;
    }
    for(UINT32 i = 0;i<0x10000;i++) {
        ((UINT32*)NosInitData->FrameBuffer.BaseAddress)[i] = 0xff00;
    }
    NosInitData->EfiRuntimeServices->ResetSystem(EfiResetCold, 0, 0, NULL);
    while(1);
}