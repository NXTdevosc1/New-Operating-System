#include <loader.h>

void Pe64RelocateImage(PE_IMAGE_HDR* PeImage, void* ImageBuffer, void* VirtualBuffer, void* ImageBase);
BOOLEAN Pe64LinkDllExports(
	PE_IMAGE_HDR* PeImage,
	char* VirtualBuffer,
	void* ProgramVirtualBuffer,
	PIMAGE_IMPORT_DIRECTORY ImportDirectory,
	void* DllVirtualBase
);
BOOLEAN Pe64LoadImports(
	PE_IMAGE_HDR* PeImage,
	void* VirtualBuffer,
	IMAGE_DATA_DIRECTORY* ImportTable
);
int BlGetPeHeaderOffset(void* hdr);
BOOLEAN BlCheckImageHeader(PE_IMAGE_HDR* hdr);

PE_IMAGE_HDR* KernelImage = NULL;
void* KernelVas = NULL;
void* KernelVirtualStart = NULL;

/*
 * BlLoadImage : Loads an executable image in the PE format located in the Buffer
 * Buffer : Executable image file buffer
 * HdrStart : The function returns the starting point of the PE Header
 * VirtualAddressSpace : The function returns the physical address of the created VAS
 * VasSize : The function returns the size of the VAS
 * BaseAddress : The actual base address located on the system space (>128 TB) where the VAS will be located
*/
BOOLEAN BlLoadImage(
    void* Buffer, // File Buffer
    PE_IMAGE_HDR** HdrStart, // Returns Header Start
    void** VirtualAddressSpace, // Returns Physical Address of the VAS
    UINT64* VasSize, // Returns VAS Size in Bytes (Page Aligned)
    void** ImageVirtualBaseAddress, // Returns Virtual Address of the loaded image
    void* ImportingImageVas, // Importing image virtual address space
    PIMAGE_IMPORT_DIRECTORY ImportingImageDir // Import directory of importing image
    ) {
	PE_IMAGE_HDR* Header;
	UINT64 VasBufferSize = 0;
	void* VasBuffer;
	EFI_PHYSICAL_ADDRESS ImageBase;
	// Header Check
	{
		int Off = BlGetPeHeaderOffset(Buffer);
		if(!Off) {
			Print(L"GET PE HDR OFFSET FAILED\n");
			
			return FALSE;
		}
		Header = (PE_IMAGE_HDR*)((char*)Buffer + Off);
		if(!BlCheckImageHeader(Header)) {
			Print(L"CheckIMG HDR FAILED\n");

			return FALSE;
		}
		*HdrStart = Header;
	}
	
	// Get virtual address buffer size
	PE_SECTION_TABLE* Sections = (PE_SECTION_TABLE*)((char*)&Header->OptionnalHeader + Header->SizeofOptionnalHeader);
	Print(L"OPT_HDR_SIZE : %d FALIGN : %d\n", Header->SizeofOptionnalHeader, Header->ThirdHeader.FileAlignment);
	for(int i = 0;i<Header->NumSections;i++) {
		if(Sections[i].Characteristics & ((PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA))) {
			if(Sections[i].VirtualSize < Sections[i].SizeofRawData) Sections[i].VirtualSize = Sections[i].SizeofRawData;
			if((Sections[i].VirtualAddress + Sections[i].VirtualSize) > VasBufferSize) VasBufferSize = Sections[i].VirtualAddress + Sections[i].VirtualSize;
			
		}
	}
	// Allocate virtual address buffer
	VasBufferSize += 0x1000;

	// Allocate 2MB Aligned pages
	ImageBase = (EFI_PHYSICAL_ADDRESS)BlAllocateSystemHeap(VasBufferSize >> 12, ImageVirtualBaseAddress);
	Print(L"Image Physical Base : %.16x , Image Base : %.16x, Entry point : %.16x\n", ImageBase, *ImageVirtualBaseAddress, Header->OptionnalHeader.EntryPointAddr);
	VasBuffer = (void*)ImageBase;
	// Copy section data to the VAS Buffer
	for(UINT32 i = 0;i<Header->NumSections;i++) {
		PE_SECTION_TABLE* Section = Sections + i;
		if (Section->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) {
			Print(L"Copying section %c%c%c%c VADDR : %.16x PTR : %.16x, SZ : %.16x\n", Section->name[0], Section->name[1], Section->name[2], Section->name[3], Section->VirtualAddress, Section->PtrToRawData, Section->VirtualSize);
			BlCopyAlignedMemory((void*)((char*)VasBuffer + Section->VirtualAddress), (UINT64*)((char*)Buffer + Section->PtrToRawData), Section->SizeofRawData);
			if (Section->VirtualSize > Section->SizeofRawData) {
				UINT64 UninitializedDataSize = Section->VirtualSize - Section->SizeofRawData;
				BlZeroAlignedMemory((void*)((char*)VasBuffer + Section->VirtualAddress + Section->SizeofRawData), UninitializedDataSize);
			}
		}
	}
	Print(L"RELOC %.16X  SZ %.16X\n", Header->OptionnalDataDirectories.BaseRelocationTable.VirtualAddress, Header->OptionnalDataDirectories.BaseRelocationTable.Size);


	if(ImportingImageVas == (void*)0x1010) {
		// This is the kernel
		ImportingImageVas = NULL;
		KernelImage = Header;
		KernelVas = VasBuffer;
		KernelVirtualStart = *ImageVirtualBaseAddress;
	}




	if(Header->OptionnalDataDirectories.BaseRelocationTable.Size) {
		Print(L"RELOCATING to %.16x...\n", *ImageVirtualBaseAddress);
		Pe64RelocateImage(Header, Buffer, VasBuffer, *ImageVirtualBaseAddress);
	}

	if(Header->OptionnalDataDirectories.ImportTable.Size) {
		// Resolve Imported Symbols
		if(!Pe64LoadImports(
			Header, VasBuffer, &Header->OptionnalDataDirectories.ImportTable
		)) {
			Print(L"FLI\n");
			return FALSE;
		}
	}
	if(Header->OptionnalDataDirectories.ExportTable.Size) {
		if(ImportingImageVas) {
			// Export symbols to parent image
				Print(L"EXPORTS\n");

			if(!Pe64LinkDllExports(
				Header,
				VasBuffer,
				ImportingImageVas,
				ImportingImageDir,
				*ImageVirtualBaseAddress
			)) return FALSE;
		} // otherwise no need to use image exported symbols
	}
	*VirtualAddressSpace = VasBuffer;
	*VasSize = VasBufferSize;
	return TRUE;
}

int BlGetPeHeaderOffset(void* hdr) {
	if (!BlisMemEqual(hdr, "MZ", 2)) return 0;
	return *(int*)((char*)hdr + 0x3c);
}

BOOLEAN BlCheckImageHeader(PE_IMAGE_HDR* hdr) {
	if (
		hdr->ThirdHeader.Subsystem != 1 ||
		hdr->MachineType != 0x8664 ||
		hdr->SizeofOptionnalHeader < sizeof(PE_OPTIONAL_HEADER) ||
		!BlisMemEqual(hdr->Signature, "PE\0\0", 4)
		) {
		return FALSE;
	}
	return TRUE;
}

void Pe64RelocateImage(PE_IMAGE_HDR* PeImage, void* ImageBuffer, void* VirtualBuffer, void* ImageBase) {
	PIMAGE_BASE_RELOCATION_BLOCK RelocationBlock = (PIMAGE_BASE_RELOCATION_BLOCK)((UINT64)VirtualBuffer + PeImage->OptionnalDataDirectories.BaseRelocationTable.VirtualAddress);
	if (!ImageBase) ImageBase = VirtualBuffer;
	UINT8* NextBlock = (UINT8*)RelocationBlock;
	UINT8* SectionEnd = NextBlock + PeImage->OptionnalDataDirectories.BaseRelocationTable.Size;
	while (NextBlock <= SectionEnd) {
		RelocationBlock = (PIMAGE_BASE_RELOCATION_BLOCK)NextBlock;
		UINT32 EntriesCount = (RelocationBlock->BlockSize - sizeof(*RelocationBlock)) / 2;
		NextBlock += RelocationBlock->BlockSize;
		if (!RelocationBlock->PageRva || !RelocationBlock->BlockSize) break;


		for (UINT32 i = 0; i < EntriesCount; i++) {
			if (RelocationBlock->RelocationEntries[i].Type != IMAGE_REL_BASED_DIR64) continue;
			UINT64* TargetRebase = (UINT64*)((char*)VirtualBuffer + RelocationBlock->PageRva + RelocationBlock->RelocationEntries[i].Offset);
			UINT64 BaseValue = *TargetRebase;

			UINT64 RawAddress = (UINT64)((char*)ImageBase + (BaseValue - PeImage->ThirdHeader.ImageBase));
			*TargetRebase = RawAddress;
		}
	}
}

/*
 * PeImage : Image Header of the DLL
 * VirtualBuffer : The VAS of the DLL
 * ProgramVirtualBuffer : The VAS of the importing program
 * ImportDirectory : Import dir of parent program
 * DllVirtualBase : Virtual Load Address of the DLL
*/
BOOLEAN Pe64LinkDllExports(
	PE_IMAGE_HDR* PeImage,
	char* VirtualBuffer,
	void* ProgramVirtualBuffer,
	PIMAGE_IMPORT_DIRECTORY ImportDirectory,
	void* DllVirtualBase
) {

		// Importing Symbols

		PIMAGE_EXPORT_DIRECTORY ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(VirtualBuffer + PeImage->OptionnalDataDirectories.ExportTable.VirtualAddress);// (DllBuffer + ExportDataSection->PtrToRawData + (PeImage->OptionnalDataDirectories.ExportTable.VirtualAddress - ExportDataSection->VirtualAddress));

		
		// BlSerialWrite("DLL Image Loaded. Linking to the target program...");

		Print(L"ETVA : %.16x, ET : %.16x\n", PeImage->OptionnalDataDirectories.ExportTable.VirtualAddress, ExportDirectory);
		Print(L"NR : %.16x, NPR : %.16x, EATR : %.16x\n", ExportDirectory->NameRva, ExportDirectory->NamePointerRva, ExportDirectory->ExportAddressTableRva);

		UINT32* NamePtr = (UINT32*)(VirtualBuffer + ExportDirectory->NamePointerRva);
		char* DllName = (char*)(VirtualBuffer + ExportDirectory->NameRva);
		char* tst = "test123";
		Print(L"Test : %a\n", tst);
		Print(L"Dll Name : %c%c%c%c\n", DllName[0], DllName[1],DllName[2],DllName[3]);
		UINT64* ImportLookupTable = (UINT64*)((char*)ProgramVirtualBuffer + ImportDirectory->ImportLookupTable);
		UINT64* ImportAddressTable = (UINT64*)((char*)ProgramVirtualBuffer + ImportDirectory->ImportAddressTableRva);

		// while(1);

		// PE_SECTION_TABLE* ExportSection = GetRvaSection(ExportDirectory->ExportAddressTableRva, PeImage);
		// if (!ExportSection) return -1;
		UINT32* ExportAddressTable = (UINT32*)(VirtualBuffer + ExportDirectory->ExportAddressTableRva);// CalculatePhysicalRva(DllBuffer, ExportSection, ExportDirectory->ExportAddressTableRva);
		UINT16* ExportOrdinalTable = (UINT16*)(VirtualBuffer + ExportDirectory->OrdinalTableRva); //(DllBuffer, ExportSection, ExportDirectory->OrdinalTableRva);

		while (*ImportLookupTable) {
			
			UINT64 LookupEntry = *ImportLookupTable;

			if (LookupEntry & IMAGE_ORDINAL_IMPORT_FLAG) {
				// Import By Ordinal
				UINT16 OrdinalNumber = IMAGE_ORDINAL_NUMBER(LookupEntry);
				UINT32 Rva = ExportAddressTable[OrdinalNumber];
				*ImportAddressTable = (UINT64)(VirtualBuffer + Rva);
			}
			else {
				// Import By name
				UINT32 HintNameRva = (UINT32)IMAGE_HINT_NAME_RVA(LookupEntry);
				PIMAGE_HINT_NAME_TABLE entry = (PIMAGE_HINT_NAME_TABLE)((char*)ProgramVirtualBuffer + HintNameRva);
				UINT64 ImportNameLen = BlStrlen(entry->Name);
				
				UINT32* n = NamePtr;
				BOOLEAN SymbolFound = FALSE;
				Print(L"Searching Symbol : %a\n", entry->Name);
				for (UINT32 i = 0; i < ExportDirectory->NumNamePointers; i++, n++) {
					char* name = (char*)(VirtualBuffer + *n);

					if (BlStrlen(name) == ImportNameLen &&
						BlisMemEqual(name, entry->Name, ImportNameLen)
						) {
						// Symbol Found
						UINT16 Ordinal = ExportOrdinalTable[i];
						UINT32 Rva = ExportAddressTable[Ordinal];
						UINT64 base = (UINT64)((UINT64)DllVirtualBase + Rva);
						Print(L"Symbol Found, base : %.16x, IAT_TARGET : %.16x, RVA : %.16x\n", base, ImportAddressTable, (UINT64)Rva);
						*ImportAddressTable = base;

						SymbolFound = TRUE;
						break;
					}
				}
				if (!SymbolFound) {
					Print(L"EXPORT SYMBOL %a NOT FOUND\n", entry->Name);
					return FALSE;
				}
			}

			ImportLookupTable++;
			ImportAddressTable++; // first is for address, second one is address of symbol name
		}
		return TRUE;
}

extern EFI_FILE_PROTOCOL* OsSystemFolder;
extern char __finf[];
BOOLEAN Pe64LoadImports(
	PE_IMAGE_HDR* PeImage,
	void* VirtualBuffer,
	IMAGE_DATA_DIRECTORY* ImportTable
) {
	UINT16 ConvertedDllName[0x100];
	PIMAGE_IMPORT_DIRECTORY ImportDirectory = (PIMAGE_IMPORT_DIRECTORY)((char*)VirtualBuffer + ImportTable->VirtualAddress);
	while(ImportDirectory->NameRva){
		// Before loading dll image is set with the target virtual address of the dll
		char* DllName = (char*)VirtualBuffer + ImportDirectory->NameRva;
		BlSerialWrite("Loading DLL :");
		BlSerialWrite(DllName);
		if(EFI_ERROR(AsciiStrToUnicodeStrS(DllName, ConvertedDllName, 0xFF))) {
			
			return FALSE;
		}
		// Find DLL File
		NOS_LIBRARY_FILE* Lib = NosInitData.Dlls;
		void* FileBuffer = NULL;
		while(Lib) {
			if(StrCmp(ConvertedDllName, Lib->FileName) == 0) {
				FileBuffer = Lib->Buffer;
				break;
			}
			Lib = Lib->Next;
		}
		if(!FileBuffer) return FALSE;
		PE_IMAGE_HDR* HdrStart;
		void* a, *b, *c; // We will not use those
		if(StrCmp(Lib->FileName, L"noskx64.exe") == 0) {
			Pe64LinkDllExports(KernelImage, KernelVas, VirtualBuffer, ImportDirectory, KernelVirtualStart);
		} else {
			// Load the DLL
			if(!BlLoadImage(
				FileBuffer, &HdrStart, &a, (UINT64*)&b, &c, VirtualBuffer, ImportDirectory
			)) {
				Print(L"Failed to import %ls , the DLL may be corrupt\n", ConvertedDllName);
				return FALSE;
			}
		}
		ImportDirectory++;
	}
	return TRUE;
}