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

char* InitErrors[] = {
    "Process management initialization failed."
};

#define RaiseInitError(ErrNum) {SerialLog(InitErrors[ErrNum]); while(1) __halt();}

void testh() {
    SerialLog("IRQ0 Fired.");
    while(1);
}

void __declspec(noreturn) NosSystemInit() {
    SerialLog("NOS_KERNEL : Kernel Booting...");

    KiInitStandardSubsystems();
    
    KiDumpPhysicalMemoryEntries(); // To determine memory length

    if(NERROR(ExCreateProcess(NULL,
    &KernelProcess,
    0,
    SUBSYSTEM_NATIVE,
    L"NOS System.",
    L"//NewOS/System/noskx64.exe",
    NosSystemInit
    ))) RaiseInitError(0);

    KernelProcess->VmSearchStart = NosInitData->NosKernelImageBase;
    KernelProcess->VmSearchEnd = (void*)-1;
    KernelProcess->PageTable = (void*)(__readcr3() & ~0xFFF);
    
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


        NSTATUS Status = KeLoadImage(
            Driver->ImageBuffer,
            Driver->ImageSize,
            NULL // Drivers reside on system process
        );

        _ui64toa(Status, bf, 0x10);
        SerialLog(bf);

        Driver->Flags |= DRIVER_LOADED;

        // Check if the driver can start in the preboot phase
        if(Driver->Flags & DRIVER_PREBOOT_LAUNCH) {
            
        }

    }
    SerialLog("drvend");

    memset(NosInitData->FrameBuffer.BaseAddress, 0xFF, NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution);
    ExInstallInterruptHandler(0, (INTERRUPT_SERVICE_HANDLER)testh, NULL);
    _enable();
    for(;;) __halt();
}