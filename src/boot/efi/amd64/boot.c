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
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* OsPartition;
UINTN NumDrives = 0;
UINTN NumPartitions = 0;

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
	if(EFI_ERROR(gBS->HandleProtocol(LoadedImage->DeviceHandle, &gEfiBlockIoProtocolGuid, (void**)&BootDrive))) {
		Print(L"Failed to get boot drive\n");
		gBS->Exit(gImageHandle, EFI_NOT_FOUND, 0, NULL);
	}

	// Get a list of all drives
	EFI_HANDLE* DriveHandles;
	if(EFI_ERROR(gBS->LocateHandleBuffer(AllHandles, &gEfiBlockIoProtocolGuid, NULL, &NumDrives, &DriveHandles))) {
		Print(L"Failed to get drive handle buffer.\n");
		gBS->Exit(gImageHandle, EFI_NOT_FOUND, 0, NULL);
	}
	Print(L"Num drive handles : %d\n", NumDrives);
	for(UINTN i = 0;i<NumDrives;i++) {
		EFI_BLOCK_IO* DriveIo;
		if(EFI_ERROR(gBS->HandleProtocol(DriveHandles[i], &gEfiBlockIoProtocolGuid, (void**)&DriveIo)) ||
		!DriveIo->Media->MediaPresent || !DriveIo->Media->LastBlock || DriveIo->Media->ReadOnly || DriveIo->Media->BlockSize != 512
		) {
			// Print(L"Failed to get drive block io\n");
		} else {

			// Print(L"BlockSize : %d , MediaId : %d, NumSectors : %llu, la : %llu\n", 
			// DriveIo->Media->BlockSize, DriveIo->Media->MediaId, DriveIo->Media->LastBlock,
			// DriveIo->Media->LowestAlignedLba
			// );
			// Read MBR
			DriveIo->ReadBlocks(DriveIo, DriveIo->Media->MediaId, 0, 512, &Mbr);
			if(Mbr.MbrSignature != MBR_SIGNATURE || Mbr.Parititons[0].FileSystemId != FSID_GPT) {
				Print(L"unsupported drive\n");
				continue;
			}
			// Read GPT
			DriveIo->ReadBlocks(DriveIo, DriveIo->Media->MediaId, Mbr.Parititons[0].StartLba, 512, &GptHeader);
			if(GptHeader.Signature != 0x5452415020494645 || GptHeader.EntrySize != sizeof(GUID_PARTITION_ENTRY) || !GptHeader.NumPartitionEntries) {
				Print(L"unsupported drive\n");
				continue;
			}
			UINT32 NumSectors = ((GptHeader.NumPartitionEntries * sizeof(GUID_PARTITION_ENTRY)) >> 9) + 1;
			if(EFI_ERROR(gBS->AllocatePool(EfiLoaderData, NumSectors << 9, (void**)&GptEntries))) {
				Print(L"Failed to allocate memory\n");
				gBS->Exit(gImageHandle, EFI_UNSUPPORTED, 0, NULL);
			}
			DriveIo->ReadBlocks(DriveIo, DriveIo->Media->MediaId, GptHeader.GptEntryStartLba, NumSectors  << 9, (void*)GptEntries);
			Print(L"GPT_LBA : %d, GPT_ENTRIES_LBA : %d, NUM_ENTRIES : %d\n", Mbr.Parititons[0].StartLba, GptHeader.GptEntryStartLba, GptHeader.NumPartitionEntries);
		
			for(UINT32 c = 0;c<GptHeader.NumPartitionEntries;c++) {

				if(BlNullGuid(GptEntries[c].PartitionType)) continue;
				Print(L"Partition : %ls, StartLba : %d, EndLba : %d\n", GptEntries[c].PartitionName, GptEntries[c].StartingLba, GptEntries[c].EndingLba);
			}
		
		}
	}
	
	

	// Disable Watchdog Timer
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	Print(L"Success.");
	while(1);
	return EFI_SUCCESS;
}