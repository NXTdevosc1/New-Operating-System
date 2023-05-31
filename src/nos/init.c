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
    
    ObInitialize();

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
    for(;;) {
        for(UINT32 i = 0;i<0xff;i++) {
            
            _Memset128A_32(NosInitData->FrameBuffer.BaseAddress, i, (NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution) / 0x10);
        }
        for(UINT32 i = 0;i<0xff;i++) {
            _Memset128A_32(NosInitData->FrameBuffer.BaseAddress, i << 8, (NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution) / 0x10);
        }
        for(UINT32 i = 0;i<0xff;i++) {
            _Memset128A_32(NosInitData->FrameBuffer.BaseAddress, i << 16, (NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution) / 0x10);
        }
    }
    _enable();
    for(;;) __halt();
}