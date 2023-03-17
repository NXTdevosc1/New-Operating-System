#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
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
 * BlNullGuid
 * Checks if the Guid is NULL ({0, 0, 0, 0})
*/

static inline BOOLEAN BlNullGuid(EFI_GUID Guid) {
	UINT64* g = (UINT64*)&Guid;
	if(g[0] == 0 && g[1] == 0) return TRUE;
	
	return FALSE;
}

/*
 * isMemEqual
 * Checks if the memory at "a" contains the same value as the memory at "b"
*/

BOOLEAN isMemEqual(void* a, void* b, UINT64 Count) {
	for(UINT64 i = 0;i<Count;i++) {
		if(((char*)a)[i] != ((char*)b)[i]) return FALSE;
	}
	return TRUE;
}


void CopyAlignedMemory(void* _dest, void* _src, UINT64 NumBytes) {
	NumBytes >>= 3;
	for(UINT64 i = 0;i<NumBytes;i++) {
		((UINT64*)_dest)[i] = ((UINT64*)_src)[i];
	}
}
void ZeroAlignedMemory(void* _dest, UINT64 NumBytes) {
	NumBytes >>= 3;
	for(UINT64 i = 0;i<NumBytes;i++) {
		((UINT64*)_dest)[i] = 0;
	}
}


/*
 * BlInitBootGraphics
 * This function sets the Graphics Output to Native Mode and claims display information
*/
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

int BlGetPeHeaderOffset(void* hdr) {
	if (!isMemEqual(hdr, "MZ", 2)) return 0;
	return *(int*)((char*)hdr + 0x3c);
}

BOOLEAN BlCheckImageHeader(PE_IMAGE_HDR* hdr) {
	if (
		hdr->ThirdHeader.Subsystem != 1 ||
		hdr->MachineType != 0x8664 ||
		hdr->SizeofOptionnalHeader < sizeof(PE_OPTIONAL_HEADER) ||
		!hdr->OptionnalHeader.EntryPointAddr ||
		!isMemEqual(hdr->Signature, "PE\0\0", 4)
		) {
		return FALSE;
	}
	return TRUE;
}



BOOLEAN BlLoadImage(void* Buffer, PE_IMAGE_HDR** HdrStart, void** VirtualAddressSpace, UINT64* VasSize) {
	PE_IMAGE_HDR* Header;
	UINT64 VasBufferSize = 0;
	void* VasBuffer;
	EFI_PHYSICAL_ADDRESS ImageBase;
	// Header Check
	{
		int Off = BlGetPeHeaderOffset(Buffer);
		if(!Off) return FALSE;
		Header = (PE_IMAGE_HDR*)((char*)Buffer + Off);
		if(!BlCheckImageHeader(Header)) return FALSE;
		*HdrStart = Header;
	}
	// Get virtual address buffer size
	PE_SECTION_TABLE* Sections = (PE_SECTION_TABLE*)((char*)Header + Header->SizeofOptionnalHeader);
	for(int i = 0;i<Header->NumSections;i++) {
		if(Sections[i].Characteristics & ((PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA))) {
			if(Sections[i].VirtualSize < Sections[i].SizeofRawData) Sections[i].VirtualSize = Sections[i].SizeofRawData;
			if(Sections[i].VirtualAddress + Sections[i].VirtualSize > VasBufferSize) VasBufferSize = Sections[i].VirtualAddress + Sections[i].VirtualSize;
		}
	}
	// Allocate virtual address buffer
	VasBufferSize += 0x10000;

	if(EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, VasBufferSize >> 12, &ImageBase))) {
		Print(L"Failed to allocate memory\n");
		gBS->Exit(gImageHandle, EFI_UNSUPPORTED, 0, NULL);
	}
	if(Header->ThirdHeader.DllCharacteristics & 0x40) {
		// TODO : Relocation
	}

	VasBuffer = (void*)ImageBase;
	// Copy section data to the VAS Buffer
	for(int i = 0;i<Header->NumSections;i++) {
		PE_SECTION_TABLE* Section = Sections + i;
		if (Section->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) {
			CopyAlignedMemory((void*)((char*)VasBuffer + Section->VirtualAddress), (UINT64*)((char*)Buffer + Section->PtrToRawData), Section->SizeofRawData);
			if (Section->VirtualSize > Section->SizeofRawData) {
				UINT64 UninitializedDataSize = Section->VirtualSize - Section->SizeofRawData;
				ZeroAlignedMemory((void*)((char*)VasBuffer + Section->VirtualAddress + Section->SizeofRawData), UninitializedDataSize);
			}
		}
	}
	*VirtualAddressSpace = VasBuffer;
	*VasSize = VasBufferSize;
	return TRUE;
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
	EFI_FILE* Kernel;
	if(EFI_ERROR(OsPartition->Open(OsPartition, &Kernel, L"NewOS\\System\\noskx64.exe", EFI_FILE_MODE_READ, 0))) {
		Print(L"noskx64.exe is missing.\n");
		return EFI_NOT_FOUND;
	}
	char __finf[sizeof(EFI_FILE_INFO) + 0x100]; // Path to kernel
	EFI_FILE_INFO* FileInfo = (EFI_FILE_INFO*)__finf;
	
	UINTN BufferSize = sizeof(EFI_FILE_INFO) + 0x100;
	
	if(EFI_ERROR(Kernel->GetInfo(Kernel, &gEfiFileInfoGuid, &BufferSize, (void*)FileInfo))) {
		Print(L"failed to get info\n");
		while(1);
	}
	BufferSize = FileInfo->FileSize;
	void* KernelBuffer;
	
	if(EFI_ERROR(gBS->AllocatePool(EfiLoaderData, BufferSize, &KernelBuffer))) {
		Print(L"Failed to allocate memory\n");
		return EFI_UNSUPPORTED;
	}
	Kernel->Read(Kernel, &BufferSize, KernelBuffer);
	PE_IMAGE_HDR* ImageHeader;
	void* Vas;
	UINT64 VasSize;
	if(!BlLoadImage(KernelBuffer, &ImageHeader, &Vas, &VasSize)) {
		Print(L"Failed to load noskx64.exe, File maybe corrupt.\n");
		return EFI_UNSUPPORTED;
	}
	
	Print(L"StartAddr : 0x%0X , Base : 0x%llu\n", ImageHeader->OptionnalHeader.EntryPointAddr, ImageHeader->ThirdHeader.ImageBase);

	// Get the memory map
	UINTN MapSize = 0, DescriptorSize = 0, MapKey = 0;
	UINT32 DescriptorVersion = 0;
	EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
	EFI_STATUS Status = gBS->GetMemoryMap(&MapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
	if(Status != EFI_BUFFER_TOO_SMALL) {
		Print(L"Failed to get memory map\n");
		return EFI_UNSUPPORTED;
	}
	MapSize += 3 * DescriptorSize;
	gBS->AllocatePool(EfiLoaderData, MapSize, (void**)&MemoryMap);
	if(EFI_ERROR(gBS->GetMemoryMap(&MapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion))) {
		Print(L"Failed to get memory map\n");
		return EFI_UNSUPPORTED;
	}

	// Disable Watchdog Timer
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	// Exit boot services
	if(EFI_ERROR(gBS->ExitBootServices(ImageHandle, MapKey))) {
		Print(L"Failed to exit boot services\n");
		return EFI_UNSUPPORTED;
	}
	if(EFI_ERROR(gST->RuntimeServices->SetVirtualAddressMap(MapSize, DescriptorSize, DescriptorVersion, MemoryMap))) {
		for(UINTN i = 0;i<0x1000;i++) {
		((UINT32*)NosInitData.FrameBuffer.BaseAddress)[i] = 0xFF00;
		while(1);
	}
	}

	for(UINTN i = 0;i<0x1000;i++) {
		((UINT32*)NosInitData.FrameBuffer.BaseAddress)[i] = 0xFF;
	}
	while(1);
	return EFI_SUCCESS;
}