/*
 * init.c
 * This file contains the initialization function of the Operating System Kernel
*/
#include <nos/nos.h>
#include <nos/loader/loader.h>

/*
 * The initialization entry point of the NOS Kernel
*/

char bf[100];

/*
- User accessible table at the top of the system space which specifies the mapping of the pages

*/


PEPROCESS KernelProcess = NULL;
extern void NOSINTERNAL KiDumpPhysicalMemoryEntries();


int  testh() {
    return 0;
}

    
void __declspec(noreturn) NosSystemInit() {
    SerialLog("NOS_KERNEL : Kernel Booting...");
    
    KiPhysicalMemoryManagerInit();
    ObInitialize();
    KiInitStandardSubsystems();
    

    

    KiInitBootCpu();
    SerialLog(NosInitData->BootHeader->OsName);
    SerialLog("drivers");
    _ui64toa(NosInitData->BootHeader->NumDrivers, bf, 10);
    SerialLog(bf);
    for(int i=0;i<NosInitData->BootHeader->NumDrivers;i++) {
        SerialLog("drv:");
        NOS_BOOT_DRIVER* Driver = NosInitData->BootHeader->Drivers + i;
        SerialLog(Driver->DriverPath);
        // Enabled flag discarded as it is already checked by bootloader
        if(!(Driver->Flags & DRIVER_LOADED)) {
            SerialLog("not loaded");
            continue;
        } else SerialLog("loaded");

        // Load the driver into memory
        Driver->Flags &= ~DRIVER_LOADED;

        NSTATUS (__cdecl* EntryPoint)(void* Driver);

        NSTATUS Status = KeLoadImage(
            Driver->ImageBuffer,
            Driver->ImageSize,
            KERNEL_MODE,
            (void**)&EntryPoint // Drivers reside on system process
        );

    memset(NosInitData->FrameBuffer.BaseAddress, 0x20, 0x28000);
    // while(1) __halt();


        _ui64toa(Status, bf, 0x10);
        SerialLog(bf);

        if(NERROR(Status)) continue;
        
        Driver->Flags |= DRIVER_LOADED;

        // Check if the driver can start in the preboot phase
        if(Driver->Flags & DRIVER_PREBOOT_LAUNCH) {
            SerialLog("Preboot Launch");
            
            Status = EntryPoint(NULL);
            SerialLog("RETURN_STATUS :");
            _ui64toa(Status, bf, 0x10);
            SerialLog(bf);
        }

    }



    SerialLog("drvend");
    
    // INTERRUPTS ARE ENABLED BY THE ACPI SUBSYSTEM IN THE FIRST CALL TO CREATE TIMER


    // Stall function requires the timer to receive interrupts

    // KeEnableScheduler();
    // for(;;) __halt();
    for(;;) {
        // for(UINT32 i = 0;i<0xff;i++) {
            UINT32 i = 0xFF;
            _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress + 0x3000, i, (NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
        // }
        Stall(100000);
        // for(UINT32 i = 0;i<0xff;i++) {
            _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress + 0x3000, i << 8, (NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
        // }
        Stall(100000);

        // for(UINT32 i = 0;i<0xff;i++) {
            _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress + 0x3000, i << 16, (NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
        // }
        Stall(100000);

    }
    for(;;) __halt();
}