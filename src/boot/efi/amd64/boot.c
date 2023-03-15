#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>

#include "../../../../inc/efi/loader.h"

/*
 * EfiInitBootGraphics
 * This function checks for GOP or UGA support and enables them
*/

NOS_INITDATA NosInitData = {0};

void BlInitBootGraphics() {
	EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsProtocol;
	// Check for G.O.P Support
	EFI_STATUS Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void**)&GraphicsProtocol);
	if(EFI_ERROR(Status)) {
		Print(L"Graphics Output Protocol is not supported.\n");
		gBS->Exit(gImageHandle, EFI_UNSUPPORTED, 0, NULL);
	}
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* ModeInfo;
	UINTN szInfo;
	Status = GraphicsProtocol->QueryMode(GraphicsProtocol, GraphicsProtocol->Mode == NULL ? 0 : GraphicsProtocol->Mode->Mode, &szInfo, &ModeInfo);
	if(Status == EFI_NOT_STARTED) {
		if(EFI_ERROR(GraphicsProtocol->SetMode(GraphicsProtocol, 0)) || !GraphicsProtocol->Mode || !GraphicsProtocol->Mode->FrameBufferBase) {
			Print(L"Failed to set video mode.\n");
			gBS->Exit(gImageHandle, EFI_UNSUPPORTED, 0, NULL);
		}
	}

	NosInitData.FrameBuffer.BaseAddress = (void*)GraphicsProtocol->Mode->FrameBufferBase;
	NosInitData.FrameBuffer.FbSize = GraphicsProtocol->Mode->FrameBufferSize;
	NosInitData.FrameBuffer.HorizontalResolution = ModeInfo->HorizontalResolution;
	NosInitData.FrameBuffer.VerticalResolution = ModeInfo->VerticalResolution;
	NosInitData.FrameBuffer.Pitch = ModeInfo->PixelsPerScanLine;
}


/*
 * NOS AMD64 UEFI Bootloader Entry Point
 * The bootloader should :
 * - Initialize Graphics Output
 * - Retreive the boot device
 * - Retreive boot partition (EfiSystemPartition) and Main Data Partition(s) (Basic Data Partition)
 * - if there are multiple partitions with multiple OSes, prompt the user to select which OS to run
 * - Load kernel and drivers
 * - Get the memory map
 * - Disable Watchdog Timer
 * - ExitBootServices
 * - Jump to the kernel
*/
EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
	// Initialize Global Variables
	gST = SystemTable;
	gBS = SystemTable->BootServices;
	gImageHandle = ImageHandle;

	// Initialize Boot Graphics
	BlInitBootGraphics();
	
	// Retreive boot device
	EFI_LOADED_IMAGE* LoadedImage;
	gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);
	
	EFI_GUID bg = BLOCK_IO_PROTOCOL;
	// EFI_FILE_IO_INTERFACE* If;
	EFI_BLOCK_IO* BlockIo;
	if(EFI_ERROR(gBS->HandleProtocol(LoadedImage->DeviceHandle, &bg, (void**)&BlockIo))) {
		Print(L"Error block io");
		while(1);
	}
	// EFI_DEVICE_PATH_PROTOCOL* Pt;

	Print(L"Id : %d , Block Size : %d\n", BlockIo->Media->MediaId, BlockIo->Media->BlockSize);
	
	// BlockIo->Media.

	// Disable Watchdog Timer before calling kernel's Entry Point
	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello World\n");

	while(1);
	return EFI_SUCCESS;
}