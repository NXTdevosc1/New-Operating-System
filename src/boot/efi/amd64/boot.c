

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
char __finf[0x2000] = {0}; // Used to request file info







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


char* tst2 = "test123";
EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
	// Initialize Global Variables
	gST = SystemTable;
	gBS = SystemTable->BootServices;
	gImageHandle = ImageHandle;

	// Eliminate NULL Page
	EFI_PHYSICAL_ADDRESS __NullPg = 0;
	gBS->AllocatePages(AllocateAddress, EfiLoaderData, 1, &__NullPg);

		Print(L"Test Str : %a\n", tst2);
	gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);
	// BlZeroAlignedMemory(&NosInitData, sizeof(NOS_INITDATA));
	// Initialize Boot Graphics
	BlInitBootGraphics();
	// Find a place to put init trampoline
	for(int i = 1;i<245 /*max page number + 8 + padding for SIPI*/;i++) {
		NosInitData.InitTrampoline = (void*)((UINT64)i << 12);
		if(EFI_ERROR(gBS->AllocatePages(AllocateAddress, EfiLoaderData, 8, (EFI_PHYSICAL_ADDRESS*)&NosInitData.InitTrampoline))) {
			if(i == 244) {
				Print(L"Failed to allocate SIPI Trampoline in address range 0x1000-0xF4000\n");
				return EFI_UNSUPPORTED;
			}
		} else break;
	}

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
	char bufff[0x1000] = {0};
	UINTN bsz = 0x1000;
	UINT16 lastfname[0x200] = {0};
	EFI_FILE* LibDir;
	if(EFI_ERROR(OsSystemFolder->Open(OsSystemFolder, &LibDir, L"Libraries", EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY))) {
		Print(L"Cannot find System/Libraries folder.\n");
		return EFI_NOT_FOUND;
	}

	NOS_LIBRARY_FILE** Set = &NosInitData.Dlls;

	for(;;) {
		EFI_FILE_INFO* info = (EFI_FILE_INFO*)bufff;
		EFI_STATUS s;
		if(EFI_ERROR((s = LibDir->Read(LibDir, &bsz, bufff)))) break;
		bsz = 0x1000;
		if(StrCmp(lastfname, info->FileName) == 0) break;
		StrCpyS(lastfname, 0x200, info->FileName);
		UINTN FileNameLength = (info->Size - 2 - SIZE_OF_EFI_FILE_INFO) >> 1;
		// Check if the file ends with .dll
		UINT16* Extension = info->FileName + FileNameLength - 4;
		if(FileNameLength < 5 || (info->Attribute & EFI_FILE_DIRECTORY) || StrCmp(Extension, L".dll") != 0) continue;

		EFI_FILE* LibraryFile;
		if(EFI_ERROR(LibDir->Open(LibDir, &LibraryFile, info->FileName, EFI_FILE_MODE_READ, 0))) {
			Print(L"failed to open %s\n", info->FileName);
			return EFI_LOAD_ERROR;
		}
		
		NOS_LIBRARY_FILE* Lib;

		if(EFI_ERROR(gBS->AllocatePool(EfiLoaderData, sizeof(NOS_LIBRARY_FILE) + info->FileSize, (void**)&Lib))) {
			Print(L"Unsufficient memory\n");
			return EFI_OUT_OF_RESOURCES;
		}
		LibraryFile->Read(LibraryFile, &info->FileSize, Lib->Buffer);
		StrCpyS(Lib->FileName, 0x100, info->FileName);
		Lib->FileSize = info->FileSize;
		Lib->Next = NULL;

		*Set = Lib;
		Set = &Lib->Next;

		LibraryFile->Close(LibraryFile);


		Print(L"Attr : %.16x Name : %s Size : %llu NL : %llu %s\n", info->Attribute, info->FileName, info->FileSize, FileNameLength, lastfname + FileNameLength - 4);
	}
	LibDir->Close(LibDir);

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

	Print(L"Kernel File Size : %d\n", BufferSize);

	if(EFI_ERROR(gBS->AllocatePool(EfiLoaderData, BufferSize + sizeof(NOS_LIBRARY_FILE), &KernelBuffer))) {
		Print(L"Failed to allocate memory\n");
		return EFI_UNSUPPORTED;
	}
	NOS_LIBRARY_FILE* KernelLib = KernelBuffer;
	KernelBuffer = (void*)(KernelLib + 1);
	Kernel->Read(Kernel, &BufferSize, KernelBuffer);

	StrCpyS(KernelLib->FileName, 255, L"noskx64.exe");
	KernelLib->FileSize = BufferSize;
	KernelLib->Next = NULL;
	*Set = KernelLib;


	char* tst = "test123";
	Print(L"Test2 : %a\n", tst);

	// Initialize System Heap
	UINTN NumSystemPages = 10; // In 2MB Pages
	BlInitSystemHeap(NumSystemPages);

	PE_IMAGE_HDR* ImageHeader;
	void* Vas;
	UINT64 VasSize;
	void* KernelBaseAddress = NULL;
	if(!BlLoadImage(KernelBuffer, &ImageHeader, &Vas, &VasSize, &KernelBaseAddress, (void*)0x1010, NULL)) {
		Print(L"Failed to load noskx64.exe, File maybe corrupt.\n");
		return EFI_UNSUPPORTED;
	}
	// Free kernel file and close it
	// gBS->FreePool(KernelBuffer); // Do not free pool kernel buffer because we still need the header
	Kernel->Close(Kernel);
	// Load boot.nos
	EFI_FILE* BootConfig;
	if(EFI_ERROR(OsPartition->Open(OsPartition, &BootConfig, L"NewOS\\System\\boot.nos", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0))) {
		Print(L"boot.nos is missing\n");
		return EFI_NOT_FOUND;
	}
	BufferSize = sizeof(EFI_FILE_INFO) + 0x100;
	if(EFI_ERROR(BootConfig->GetInfo(BootConfig, &gEfiFileInfoGuid, &BufferSize, (void*)FileInfo))) {
		Print(L"boot.nos is missing\n");
		return EFI_NOT_FOUND;
	}

	NOS_BOOT_HEADER* BootHeader;
	if(EFI_ERROR(gBS->AllocatePool(EfiLoaderData, FileInfo->FileSize, (void**)&BootHeader))) {
		Print(L"Unsufficient memory\n");
		return EFI_UNSUPPORTED;
	}
	BufferSize = FileInfo->FileSize;
	if(EFI_ERROR(BootConfig->Read(BootConfig, &BufferSize, BootHeader))) {
		Print(L"Failed to read file\n");
		return EFI_UNSUPPORTED;
	}

	BootConfig->Close(BootConfig);
	// Load boot drivers
	NOS_BOOT_DRIVER* Driver = BootHeader->Drivers;
	UINT16 conv[256];
	for(UINTN i = 0;i<BootHeader->NumDrivers;i++, Driver++) {
		Driver->Flags &= ~DRIVER_LOADED;
		if(Driver->Flags & DRIVER_ENABLED) {
			EFI_FILE* DriverFile;
			AsciiStrToUnicodeStrS((const CHAR8*)Driver->DriverPath, conv, 256);
			if(EFI_ERROR(OsPartition->Open(
				OsPartition, &DriverFile,
				conv,
				EFI_FILE_MODE_READ, 0
			))) {
				Print(L"Cannot find : %s\n", conv);
				continue;
			}
			BufferSize = sizeof(EFI_FILE_INFO) + 0x1000;
			if(EFI_ERROR(DriverFile->GetInfo(
				DriverFile, &gEfiFileInfoGuid, &BufferSize, FileInfo
			))) {
				DriverFile->Close(DriverFile);
				Print(L"Failed to load: %s\n", conv);
				continue;
			}

			Driver->ImageSize = FileInfo->FileSize;
			if(EFI_ERROR(gBS->AllocatePool(
				EfiLoaderData, Driver->ImageSize, &Driver->ImageBuffer
			))) {
				Print(L"No enough memory to load %ls\n", conv);
				DriverFile->Close(DriverFile);
				return EFI_UNSUPPORTED;
			}
			BufferSize = Driver->ImageSize;
			if(EFI_ERROR(DriverFile->Read(
				DriverFile, &BufferSize, Driver->ImageBuffer
			))) {
				DriverFile->Close(DriverFile);
				continue;
			}
			DriverFile->Close(DriverFile);
			Driver->Flags |= DRIVER_LOADED;
			Print(L"%s loaded\n");
		}
	}


	NosInitData.BootHeader = BootHeader;
	NosInitData.NosFile = (void*)ImageHeader;
	NosInitData.NosKernelImageBase = KernelBaseAddress;
	NosInitData.NosKernelImageSize = VasSize;

	NosInitData.NumConfigurationTableEntries = SystemTable->NumberOfTableEntries;
	NosInitData.EfiConfigurationTable = SystemTable->ConfigurationTable;

	if(BootHeader->Magic != NOS_BOOT_MAGIC) {
		Print(L"nos.boot is corrupt, please re-install the operating system.\n");
		return EFI_UNSUPPORTED;
	}
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
	BlSerialWrite("Memory Count:");
	BlSerialWrite(BlToStringUint64(NosInitData.MemoryCount));
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
	
	
	BlSerialWrite("Booted successfully.");
	NOS_ENTRY_POINT NosEntryPoint = (NOS_ENTRY_POINT)((UINT64)KernelBaseAddress + (UINT64)ImageHeader->OptionnalHeader.EntryPointAddr);
	BlSerialWrite(BlToHexStringUint64((UINT64)NosEntryPoint));
	
	BlSerialWrite(BlToHexStringUint64(*(UINT64*)NosEntryPoint));


	NosEntryPoint(&NosInitData); // INIT_DATA Passed through RDI using GCC Calling Convention
	BlSerialWrite("ERROR : NOS_RETURNED");
	while(1) asm ("hlt");
	return EFI_UNSUPPORTED;
}