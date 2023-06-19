/*
 * init.c
 * This file contains the initialization function of the Operating System Kernel
*/
#include <nos/nos.h>
#include <nos/loader/loader.h>
#include <nos/processor/hw.h>

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

extern void HwInitTrampoline();

extern inline void DrawRect(UINT x, UINT y, UINT Width, UINT Height, UINT Color) {
    for(UINT a = y; a<y+Height;a++) {
        _Memset128U_32((char*)NosInitData->FrameBuffer.BaseAddress + x * 4 + a * NosInitData->FrameBuffer.Pitch, Color, Width >> 2);
    }
}

void thread1() {
    for(;;)
        DrawRect(20, 20, 100, 100, 0xFFFFFF);

}

void thread2() {
    for(;;)
        DrawRect(20, 20, 100, 100, 0xFF);

}

UINT32 errcolors[] = {
    0,
    0xFFFFFF,
    0xFF,
    0xFF00,
    0xFF0000,
    0xFFFF,
    0xFFFF00  
};

    
void NOSENTRY NosSystemInit() {
    SerialLog("NOS_KERNEL : Kernel Booting...");
    
    KiPhysicalMemoryManagerInit();
    ObInitialize();
    KiInitBootCpu();
    KiInitStandardSubsystems();
    

    

    SerialLog(NosInitData->BootHeader->OsName);
    SerialLog("drivers");
    _ui64toa(NosInitData->BootHeader->NumDrivers, bf, 10);
    SerialLog(bf);

    DrawRect(0, 0, 100, 100, 0x43829F0);

    // ConClear();

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
            KDebugPrint("Running driver %x EntryPoint %x", i, EntryPoint);
            Status = EntryPoint(NULL);
            SerialLog("RETURN_STATUS :");
            _ui64toa(Status, bf, 0x10);
            SerialLog(bf);

            if(NERROR(Status)) {
                if(Status > 6) Status = 6;
                _disable();
                DrawRect(0, 0, 500, 500, errcolors[Status]);
                DrawRect(0, 0, 100, 100, 0xFFFFFF);

                while(1) __halt();
            }
        }

    }



    SerialLog("drvend");
    
    // INTERRUPTS ARE ENABLED BY THE ACPI SUBSYSTEM IN THE FIRST CALL TO CREATE TIMER


    // Stall function requires the timer to receive interrupts

    
    // Setup init trampoline
    {
        KDebugPrint("INIT_AP_TRAMPOLINE at 0x%x, SymbolStart : %x", NosInitData->InitTrampoline, HwInitTrampoline);
        // // Copy trampoline
        KeMapVirtualMemory(KernelProcess, NosInitData->InitTrampoline, NosInitData->InitTrampoline, 8, PAGE_WRITE_ACCESS | PAGE_EXECUTE, PAGE_CACHE_DISABLE);
        memcpy(NosInitData->InitTrampoline, HwInitTrampoline, 0x8000);
        // Rebase trampolie
        UINT64 base = (UINT64)NosInitData->InitTrampoline;
        // page table
        *(UINT64*)(base + 0x1000) += base;
        *(UINT64*)(base + 0x2000) += base;
        // gdtr
        *(UINT64*)(base + 0x4002) += base;
        // page table
        *(UINT64*)(base + 0x6000) = (UINT64)KernelProcess->PageTable;
        __wbinvd();

    }

    // Start using APIC ID to get a processor
    BootProcessor->ProcessorEnabled = TRUE;
    
    UINT64 _ev = 0;
    POBJECT Out;
    while((_ev = ObEnumerateObjects(NULL, OBJECT_PROCESSOR, &Out, NULL, _ev)) != 0) {
        KDebugPrint("Object %x", Out);
        RFPROCESSOR Processor = Out->Address;
        KDebugPrint("Processor %x", Processor);
        KDebugPrint("Processor#%d Characteristics : %x", Processor->Id.ProcessorId, Processor->Id.Characteristics);

        // Init the processor
        if(Processor->Id.Characteristics & PROCESSOR_BOOTABLE) {
            HwBootProcessor(Processor);
        }
    }

    // for(;;);
    // KeEnableScheduler();
    // for(;;) __halt();
    // _disable();
    PETHREAD t1, t2;
    KeCreateThread(KernelProcess, &t1, 0, thread1, NULL);
    KeCreateThread(KernelProcess, &t2, 0, thread2, NULL);

    for(;;) {
        // for(UINT32 i = 0;i<0xff;i++) {
            UINT32 i = 0xFF;
            _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress + 0x3000, i, (NosInitData->FrameBuffer.Pitch * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
        // }
        // Stall(1000000);
        // for(UINT32 i = 0;i<0xff;i++) {
            _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress + 0x3000, i << 8, (NosInitData->FrameBuffer.Pitch * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
        // }
        // Stall(1000000);

        // for(UINT32 i = 0;i<0xff;i++) {
            _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress + 0x3000, i << 16, (NosInitData->FrameBuffer.Pitch * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
        // }
        // Stall(1000000);

    }
    for(;;) __halt();
}

void KRNLAPI __KiClearScreen(UINT Color) {
    _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress, Color, (NosInitData->FrameBuffer.Pitch * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
}