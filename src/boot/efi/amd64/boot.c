#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include "../../../../inc/efi/loader.h"

NOS_INITDATA NosInitData = {0};
EFI_LOADED_IMAGE* LoadedImage;
EFI_BLOCK_IO_PROTOCOL* BootDrive;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* BootPartition;
EFI_HANDLE OsPartitionHandle;
EFI_FILE_PROTOCOL* OsPartition;
#define MAX_OS_PARTITIONS 20
EFI_HANDLE _OsPartitions[MAX_OS_PARTITIONS];
EFI_FILE_PROTOCOL* _OsRoots[MAX_OS_PARTITIONS];
UINTN NumDrives = 0;
UINTN NumPartitions = 0;
UINTN NumOsPartitions = 0;

MASTER_BOOT_RECORD Mbr;
GUID_PARTITION_TABLE_HEADER GptHeader;
GUID_PARTITION_ENTRY* GptEntries;



/*
 * BlInitBootGraphics
 * This function sets the Graphics Output to Native Mode and claims display information
*/

static inline BOOLEAN BlNullGuid(EFI_GUID Guid) {
	UINT64* g = (UINT64*)&Guid;
	if(g[0] == 0 && g[1] == 0) return TRUE;
	
	return FALSE;
}

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
 * - Retreive the boot device & partition
 * - Query all partitions
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
	gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);

	// Initialize Boot Graphics
	BlInitBootGraphics();
	
	// Query the boot device & partition
	if(EFI_ERROR(gBS->HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&BootPartition))) {
		Print(L"Failed to get boot partition\n");
		gBS->Exit(gImageHandle, EFI_NOT_FOUND, 0, NULL);
	}
	

	// Get a list of all partitions
	EFI_HANDLE* PartitionHandles;
	if(EFI_ERROR(gBS->LocateHandleBuffer(AllHandles, &gEfiSimpleFileSystemProtocolGuid, NULL, &NumPartitions, &PartitionHandles))) {
		Print(L"Failed to get partition handle buffer.\n");
		gBS->Exit(gImageHandle, EFI_NOT_FOUND, 0, NULL);
	}
	Print(L"Num partition handles : %d\n", NumPartitions);
	for(UINTN i = 0;i<NumPartitions;i++) {
		EFI_FILE_IO_INTERFACE* FileIo;
		EFI_FILE_PROTOCOL* Root;
		if(!EFI_ERROR(gBS->HandleProtocol(PartitionHandles[i], &gEfiSimpleFileSystemProtocolGuid, (void**)&FileIo))
		&& !EFI_ERROR(FileIo->OpenVolume(FileIo, &Root))
		) {
			Print(L"Data partition found.\n");
			EFI_FILE_PROTOCOL* OsDir;
			if(EFI_ERROR(Root->Open(Root, &OsDir, L"NewOS\\", EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY))) {
				Print(L"No os was found on the partition\n");
				continue;
			}
			_OsPartitions[NumOsPartitions] = PartitionHandles[i];
			_OsRoots[NumOsPartitions] = Root;
			NumOsPartitions++;
			if(NumOsPartitions == MAX_OS_PARTITIONS) break;
		}
	}

	if(!NumOsPartitions) {
		Print(L"The Operating System doesn't seem to be found.\n");
		return EFI_LOAD_ERROR;
	}
	if(NumOsPartitions > 1) {
		// TODO : Prompt the user to select which OS needs to be run.
		Print(L"Multiple Operating Systems were found, please choose an OS to run :\n");
		while(1);
	} else {
		OsPartitionHandle = _OsPartitions[0];
		OsPartition = _OsRoots[0];
		// Get parent drive of the partition
		if(EFI_ERROR(gBS->HandleProtocol(OsPartitionHandle, &gEfiBlockIoProtocolGuid, (void**)&BootDrive))) {
			Print(L"Failed to get boot drive\n");
			gBS->Exit(gImageHandle, EFI_NOT_FOUND, 0, NULL);
		}
		Print(L"Boot partition and boot drive selected successfully.\n");
	}

	// Load the kernel (TODO : Load Modules and drivers)

	

	// Disable Watchdog Timer
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	Print(L"Success.");
	while(1);
	return EFI_SUCCESS;
}