

#include <loader.h>
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
	// ZeroAlignedMemory(&NosInitData, sizeof(NOS_INITDATA));
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
	void* KernelBaseAddress = (void*)0xffff800000000000;
	if(!BlLoadImage(KernelBuffer, &ImageHeader, &Vas, &VasSize, KernelBaseAddress)) {
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
	NosInitData.MemoryCount = MapSize / DescriptorSize;
	MapSize += 3 * DescriptorSize;
	gBS->AllocatePool(EfiLoaderData, MapSize, (void**)&MemoryMap);
	
	if(EFI_ERROR(gBS->GetMemoryMap(&MapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion))) {
		Print(L"Failed to get memory map\n");
		return EFI_UNSUPPORTED;
	}

	NosInitData.MemoryMap = MemoryMap;
	NosInitData.MemoryDescriptorSize = DescriptorSize;


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
	QemuWriteSerialMessage("Memory Count:");
	QemuWriteSerialMessage(ToStringUint64(NosInitData.MemoryCount));
	// Fill NOS Memory Linked List with data
	for(UINTN i = 0;i<NosInitData.MemoryCount;i++) {
		EFI_MEMORY_DESCRIPTOR* Desc = (EFI_MEMORY_DESCRIPTOR*)((char*)NosInitData.MemoryMap + DescriptorSize * i);
		if(Desc->Type == EfiConventionalMemory) {
			BlAllocateMemoryDescriptor(Desc->PhysicalStart, Desc->NumberOfPages, FALSE);
		} else if(Desc->Type == EfiLoaderCode || Desc->Type == EfiLoaderData) {
			BlAllocateMemoryDescriptor(Desc->PhysicalStart, Desc->NumberOfPages, TRUE);
		}
	}
	QemuWriteSerialMessage("Booted successfully.");
	
	BlInitPageTable();
	
	BlMapToSystemSpace(Vas, Convert2MBPages(VasSize));
	for(UINTN i = 0;i<0x1000;i++) {
		((UINT32*)NosInitData.FrameBuffer.BaseAddress)[i] = 0xFF;
	}
	while(1);
	return EFI_SUCCESS;
}