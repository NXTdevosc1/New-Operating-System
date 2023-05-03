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
    KeMapVirtualMemory(
        KernelProcess,
        (void*)0x1000,
        (void*)0xffff800500200000,
        2,
        PAGE_USER | PAGE_WRITE_ACCESS | PAGE_GLOBAL,
        0
    );
    KeMapVirtualMemory(
        KernelProcess,
        (void*)0x14000,
        (void*)0xffff800500202000,
        2,
        PAGE_WRITE_ACCESS | PAGE_GLOBAL,
        0
    );
    UINT64 np = 10;
    // KeUnmapVirtualMemory(
    //     KernelProcess,
    //     (void*)0xffff800500202000,
    //     &np
    // );

    KDebugPrint("Check mem access (BOOL, FLAGS)");
    BOOLEAN b = KeCheckMemoryAccess(
        KernelProcess,
        (void*)0xffff800500200000,
        0x3000,
        &np
    );
    _ui64toa(b, bf, 0x10);
    SerialLog(bf);
    _ui64toa(np, bf, 0x10);
    SerialLog(bf);

    _ui64toa((UINT64)KeConvertPointer(KernelProcess, (void*)0xffff800500200000), bf, 0x10);
    SerialLog(bf);
    _ui64toa((UINT64)KeConvertPointer(KernelProcess, (void*)0xffff800500203000), bf, 0x10);
    SerialLog(bf);
    GUID g = EFI_ACPI_20_TABLE_GUID;
    _ui64toa((UINT64)KeFindSystemFirmwareTable("RSD PTR ", &g), bf, 0x10);
    SerialLog(bf);
    
    _ui64toa((UINT64)KeFindAvailableAddressSpace(KernelProcess, 0x10, (void*)0xffff800000000000, (void*)0xffff800500200000, 0), bf, 0x10);
    ProcessReleaseControlLock(KernelProcess, PROCESS_CONTROL_ALLOCATE_ADDRESS_SPACE);
    SerialLog(bf);
    _ui64toa((UINT64)KeFindAvailableAddressSpace(KernelProcess, 0x10, (void*)0x1000, (void*)0xffff800500200000, PAGE_2MB), bf, 0x10);
    SerialLog(bf);
    ProcessReleaseControlLock(KernelProcess, PROCESS_CONTROL_ALLOCATE_ADDRESS_SPACE);

    for(UINT i = 0;i<500000;i++) {
        MmAllocateMemory(KernelProcess, 1, PAGE_WRITE_ACCESS);
    }

_ui64toa((UINT64)MmAllocateMemory(KernelProcess, 100, PAGE_2MB), bf, 0x10);
    SerialLog(bf);
    _ui64toa((UINT64)MmAllocateMemory(KernelProcess, 1, PAGE_WRITE_ACCESS), bf, 0x10);
    SerialLog(bf);
    memset(NosInitData->FrameBuffer.BaseAddress, 0, NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution);

    KDebugPrint("Test2 : %c", 'A');
    // KiDumpPhysicalMemoryEntries();

    // NosInitData->EfiRuntimeServices->ResetSystem(EfiResetCold, 0, 0, NULL);
    for(;;) __halt();
}