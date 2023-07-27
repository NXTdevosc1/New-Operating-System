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

void thread3() {
    // Testing thread exit functionnality
    KDebugPrint("THREAD3 Created, testing thread exit functionnality");
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

NSTATUS DrvEvt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access) {
    return STATUS_SUCCESS;
}

__declspec(dllexport) NOS_INITDATA* __fastcall KiGetInitData() {
    return NosInitData;
}
    
void NOSENTRY NosSystemInit() {
    SerialLog("NOS_KERNEL : Kernel Booting...");
    
    KiPhysicalMemoryManagerInit();
    ObInitialize();
    KiInitBootCpu();
    KiInitStandardSubsystems();
    

    PETHREAD kInitThread = KeGetCurrentThread();
    KeSetThreadPriority(kInitThread, THREAD_PRIORITY_HIGH);
    SerialLog(NosInitData->BootHeader->OsName);
    SerialLog("drivers");
    _ui64toa(NosInitData->BootHeader->NumDrivers, bf, 10);
    SerialLog(bf);

    DrawRect(0, 0, 100, 100, 0x43829F0);

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

    // ConClear();
    KDebugPrint("Running PRE BOOT LAUNCH Drivers");

    for(int i=0;i<NosInitData->BootHeader->NumDrivers;i++) {
        NOS_BOOT_DRIVER* Driver = NosInitData->BootHeader->Drivers + i;
        KDebugPrint("Driver %s", Driver->DriverPath);
        // Enabled flag discarded as it is already checked by bootloader
        if(!(Driver->Flags & DRIVER_LOADED)) {
            SerialLog("not loaded");
            continue;
        } else SerialLog("loaded");

        // Load the driver into memory
        Driver->Flags &= ~DRIVER_LOADED;

        NSTATUS (__cdecl* EntryPoint)(PDRIVER Driver) = NULL;

// Constructing driver object
        POBJECT ObjectHeader;

        if(NERROR(ObCreateObject(
            NULL, &ObjectHeader, OBJECT_PERMANENT, OBJECT_DRIVER, Driver->DriverPath, sizeof(DRIVER), DrvEvt
        ))) {
            KDebugPrint("Failed to create driver object.");
            while(1) __halt();
        }

        PDRIVER DriverObject = ObjectHeader->Address;
        DriverObject->DriverId = ObjectHeader->ObjectId;
        DriverObject->ObjectHeader = ObjectHeader;

        NSTATUS Status = KeLoadImage(
            Driver->ImageBuffer,
            Driver->ImageSize,
            KERNEL_MODE,
            ObjectHeader,
            (void**)&EntryPoint // Drivers reside on system process
        );
        _ui64toa(Status, bf, 0x10);
        SerialLog(bf);

        if(NERROR(Status)) {
            KDebugPrint("Failed to load driver image.");
            KeRaiseException(Status);   
        }
        Driver->Flags |= DRIVER_LOADED;

        DriverObject->ImageFile = Driver->ImageBuffer;
        Driver->ImageBuffer = (void*)DriverObject; // The ib is now the object itself

        DriverObject->EntryPoint = EntryPoint;

        if(NERROR(ObOpenHandle(ObjectHeader, KernelProcess, HANDLE_ALL_ACCESS, &DriverObject->DriverHandle))) {
            KDebugPrint("Failed to open driver handle.");
            while(1) __halt();
        }

        DriverObject->SystemProcess = KernelProcess;


        // Check if the driver can start in the preboot phase
        if(Driver->Flags & DRIVER_PREBOOT_LAUNCH) {
            SerialLog("Preboot Launch");
            KDebugPrint("Running driver %x EntryPoint %x", DriverObject->DriverId, EntryPoint);
            Status = KiInitLibraries(DriverObject);
            if(NERROR(Status)) {
                KDebugPrint("RUN DRIVER LIBS FAILED.");
                while(1) __halt();
            }
            kInitThread->RunningDriver = DriverObject;
            Status = EntryPoint(DriverObject);
            kInitThread->RunningDriver = NULL;
            SerialLog("RETURN_STATUS :");
            _ui64toa(Status, bf, 0x10);
            SerialLog(bf);

                KDebugPrint("%ls : TID %d PID %d PRID %d", KeGetCurrentThread()->Process->ProcessDisplayName, KeGetCurrentThreadId(), KeGetCurrentProcessId(), KeGetCurrentProcessorId());
            while(1) {
            }

            if(NERROR(Status)) {
                if(Status > 6) Status = 6;
                _disable();
                DrawRect(0, 0, 500, 500, errcolors[Status]);
                DrawRect(0, 0, 100, 100, 0xFFFFFF);

                while(1) __halt();
            }
        }

    }

    // Now Run AUTO BOOT LAUNCH Drivers
    KDebugPrint("Running AUTO BOOT LAUNCH Drivers");
    for(int i=0;i<NosInitData->BootHeader->NumDrivers;i++) {
        NOS_BOOT_DRIVER* Driver = NosInitData->BootHeader->Drivers + i;
        // Enabled flag discarded as it is already checked by bootloader
        if(!(Driver->Flags & DRIVER_LOADED)) {
            continue;
        }
        PDRIVER DriverObject = (PDRIVER)Driver->ImageBuffer;
        if(Driver->Flags & DRIVER_BOOT_LAUNCH) {
            KDebugPrint("BOOT_LAUNCH Driver");
            KDebugPrint("Running Driver#%d EntryPoint %x", DriverObject->DriverId, DriverObject->EntryPoint);
            
            NSTATUS Status = KiInitLibraries(DriverObject);
            if(NERROR(Status)) {
                KDebugPrint("RUN DRIVER LIBS FAILED.");
                while(1) __halt();
            }
            kInitThread->RunningDriver = DriverObject;
            
            Status = DriverObject->EntryPoint(DriverObject);
            kInitThread->RunningDriver = NULL;

            KDebugPrint("Return status : %d", Status);
            if(NERROR(Status)) {
                _disable();
                KDebugPrint("Driver startup failed");
                while(1) __halt();
            }
        }
    }

    NOS_LIBRARY_FILE* Lib = NosInitData->Dlls;
    while(Lib) {
        KDebugPrint("LIB : %s SZ : %x", Lib->FileName, Lib->FileSize);
        Lib = Lib->Next;
    }

    SerialLog("drvend");
    
    // INTERRUPTS ARE ENABLED BY THE ACPI SUBSYSTEM IN THE FIRST CALL TO CREATE TIMER


    // Stall function requires the timer to receive interrupts

    
    

    
    PETHREAD t1, t2;
    // KeCreateThread(KernelProcess, &t1, 0, thread1, NULL);
    // KeCreateThread(KernelProcess, &t2, 0, thread2, NULL);
    // KeCreateThread(KernelProcess, &t2, 0, thread3, NULL);
    // KeCreateThread(KernelProcess, &t2, 0, thread3, NULL);
    // KeCreateThread(KernelProcess, &t2, 0, thread3, NULL);

    // for(;;);
    // KeEnableScheduler();
    // for(;;) __halt();
    // _disable();

    KDebugPrint("Drivers runned successfully, physical devices:");

    PDEVICE dev;
    UINT64 _ev = 0;
    while(KeEnumerateDevices(NULL, &dev, NULL, FALSE, &_ev)) {
        if(dev->DeviceType != VIRTUAL_DEVICE) {
            KDebugPrint("Physical Device#%u Type %u : %ls", dev->ObjectDescriptor->ObjectId, dev->DeviceType, dev->DisplayName);
        }
    }
    KDebugPrint("Virtual devices:");
    _ev = 0;
    while(KeEnumerateDevices(NULL, &dev, NULL, FALSE, &_ev)) {
        if(dev->DeviceType == VIRTUAL_DEVICE) {
            KDebugPrint("Virtual Device#%u Type %u : %ls", dev->ObjectDescriptor->ObjectId, dev->DeviceType, dev->DisplayName);
        }
    }

    for(;;) {
        KDebugPrint("KTHREADS %u", KernelProcess->NumberOfThreads);
        // for(UINT32 i = 0;i<0xff;i++) {
            UINT32 i = 0xFF;
            // _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress + 0x3000, i, (NosInitData->FrameBuffer.Pitch * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
        // }
        Sleep(1000);
        continue;
        // for(UINT32 i = 0;i<0xff;i++) {
            // _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress + 0x3000, i << 8, (NosInitData->FrameBuffer.Pitch * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
        // }
        Sleep(1000);
        // Stall(1000000);

        // for(UINT32 i = 0;i<0xff;i++) {
            // _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress + 0x3000, i << 16, (NosInitData->FrameBuffer.Pitch * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
        // }
        Sleep(1000);

        // Stall(1000000);

    }
    for(;;) __halt();
}

void KRNLAPI __KiClearScreen(UINT Color) {
    _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress, Color, (NosInitData->FrameBuffer.Pitch * NosInitData->FrameBuffer.VerticalResolution) / 0x20);
}