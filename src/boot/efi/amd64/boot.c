#include "../../../../inc/efi/loader.h"

/*
 * NOS AMD64 UEFI Bootloader Entry Point
*/
EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
	// Initialize Global Variables
	gST = SystemTable;
	gBS = SystemTable->BootServices;
	gImageHandle = ImageHandle;




	// Disable Watchdog Timer before calling kernel's Entry Point
	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

	return EFI_SUCCESS;
}