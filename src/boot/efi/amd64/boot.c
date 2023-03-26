

#include <loader.h>
NOS_INITDATA NosInitData = {0};
EFI_LOADED_IMAGE* LoadedImage = NULL;
EFI_BLOCK_IO_PROTOCOL* BootDrive = NULL;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* BootPartition = NULL;
EFI_HANDLE OsPartitionHandle = NULL;
EFI_FILE_PROTOCOL* OsPartition = NULL;
EFI_FILE_PROTOCOL* OsSystemFolder = NULL;
#define MAX_OS_PARTITIONS 20
EFI_HANDLE _OsPartitions[MAX_OS_PARTITIONS] = {0};
EFI_FILE_PROTOCOL* _OsRoots[MAX_OS_PARTITIONS] = {0};
UINTN NumDrives = 0;
UINTN NumPartitions = 0;
UINTN NumOsPartitions = 0;
char __finf[sizeof(EFI_FILE_INFO) + 0x200] = {0}; // Used to request file info







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
	// Open System Folder
	if(EFI_ERROR(OsPartition->Open(OsPartition, &OsSystemFolder, L"NewOS\\System\\", EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY))) {
		Print(L"Cannot find System Folder.\n");
		return EFI_NOT_FOUND;
	}

	// Load the kernel (TODO : Load Modules and drivers)
	EFI_FILE* Kernel;
	if(EFI_ERROR(OsSystemFolder->Open(OsSystemFolder, &Kernel, L"noskx64.exe", EFI_FILE_MODE_READ, 0))) {
		Print(L"noskx64.exe is missing.\n");
		return EFI_NOT_FOUND;
	}
	
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

	// Initialize System Heap
	UINTN NumSystemPages = 1; // In 2MB Pages
	BlInitSystemHeap(NumSystemPages);

	PE_IMAGE_HDR* ImageHeader;
	void* Vas;
	UINT64 VasSize;
	void* KernelBaseAddress = NULL;
	if(!BlLoadImage(KernelBuffer, &ImageHeader, &Vas, &VasSize, &KernelBaseAddress, NULL, NULL)) {
		Print(L"Failed to load noskx64.exe, File maybe corrupt.\n");
		return EFI_UNSUPPORTED;
	}
	// Free kernel file and close it
	gBS->FreePool(KernelBuffer);
	Kernel->Close(Kernel);
	NosInitData.NosPhysicalBase = Vas;
	NosInitData.NosKernelImageBase = KernelBaseAddress;
	NosInitData.NosKernelImageSize = VasSize;
	
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

	NosInitData.MemoryCount = MapSize / DescriptorSize;
	NosInitData.MemoryMap = MemoryMap;
	NosInitData.MemoryDescriptorSize = DescriptorSize;


	// Disable Watchdog Timer
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	// Exit boot services
	if(EFI_ERROR(gBS->ExitBootServices(ImageHandle, MapKey))) {
		Print(L"Failed to exit boot services\n");
		return EFI_UNSUPPORTED;
	}
	SerialWrite("Memory Count:");
	SerialWrite(ToStringUint64(NosInitData.MemoryCount));
	// Fill NOS Memory Linked List with data
	for(UINTN i = 0;i<NosInitData.MemoryCount;i++) {
		EFI_MEMORY_DESCRIPTOR* Desc = (EFI_MEMORY_DESCRIPTOR*)((char*)NosInitData.MemoryMap + NosInitData.MemoryDescriptorSize * i);
		if(Desc->Type == EfiConventionalMemory) {
			BlAllocateMemoryDescriptor(Desc->PhysicalStart, Desc->NumberOfPages, FALSE);
		} else if(Desc->Type == EfiLoaderCode || Desc->Type == EfiLoaderData) {
			BlAllocateMemoryDescriptor(Desc->PhysicalStart, Desc->NumberOfPages, TRUE);
		}
	}
	asm volatile ("cli");
	BlInitPageTable();
	
	// Set virtual address map
	if(EFI_ERROR(gST->RuntimeServices->SetVirtualAddressMap(MapSize, DescriptorSize, DescriptorVersion, MemoryMap))) {
		for(UINTN i = 0;i<0x1000;i++) {
		((UINT32*)NosInitData.FrameBuffer.BaseAddress)[i] = 0xFF00;
		while(1);
		}
	}
	NosInitData.EfiRuntimeServices = gST->RuntimeServices;
	
	
	SerialWrite("Booted successfully.");
	
	NOS_ENTRY_POINT NosEntryPoint = (NOS_ENTRY_POINT)((UINT64)KernelBaseAddress + (UINT64)ImageHeader->OptionnalHeader.EntryPointAddr);


	NosEntryPoint(&NosInitData); // INIT_DATA Passed through RDI using GCC Calling Convention
	SerialWrite("ERROR : NOS_RETURNED");
	while(1) asm ("hlt");
	return EFI_UNSUPPORTED;
}