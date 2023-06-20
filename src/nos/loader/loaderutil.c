#include <nos/nos.h>
#include <nos/loader/loaderutil.h>
NSTATUS LoaderRelocateImage(
    IMAGE_NT_HEADERS* Image,
    void* ImageFile,
    void* VaBuffer,
    void* NewBaseAddress
) {
    PIMAGE_BASE_RELOCATION_BLOCK RelocationBlock = (PIMAGE_BASE_RELOCATION_BLOCK)((UINT64)VaBuffer + Image->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
	UINT8* NextBlock = (UINT8*)RelocationBlock;
	UINT8* SectionEnd = NextBlock + Image->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
	while (NextBlock <= SectionEnd) {
		RelocationBlock = (PIMAGE_BASE_RELOCATION_BLOCK)NextBlock;
		UINT32 EntriesCount = (RelocationBlock->BlockSize - sizeof(*RelocationBlock)) / 2;
		NextBlock += RelocationBlock->BlockSize;
		if (!RelocationBlock->PageRva || !RelocationBlock->BlockSize) break;


		for (UINT32 i = 0; i < EntriesCount; i++) {
			if (RelocationBlock->RelocationEntries[i].Type != IMAGE_REL_DIR64) continue;
			UINT64* TargetRebase = (UINT64*)((char*)VaBuffer + RelocationBlock->PageRva + RelocationBlock->RelocationEntries[i].Offset);
			UINT64 BaseValue = *TargetRebase;

			UINT64 RawAddress = (UINT64)((char*)NewBaseAddress + (BaseValue - Image->OptionalHeader.ImageBase));
			*TargetRebase = RawAddress;
		}
	}
    return STATUS_SUCCESS;
}
#define IMAGE_ORDINAL_IMPORT_FLAG 0x8000000000000000
#define IMAGE_HINT_NAME_RVA(LookupEntry) (LookupEntry & (0x7fffffff));
#define IMAGE_ORDINAL_NUMBER(LookupEntry) (LookupEntry & 0xffff)
static char __fmtloader[100] = {0};

#define HexPrint64(Num) {_ui64toa(Num, __fmtloader, 0x10); SerialLog(__fmtloader);} 


NSTATUS LoaderImportLibrary(
	IMAGE_NT_HEADERS* Image,
	PIMAGE_IMPORT_DIRECTORY ImportDir,
	void* VaBuffer,
	char* DllName
) {
	IMAGE_NT_HEADERS* ImageHeader = NosInitData->NosFile;
	void* VasDll = NosInitData->NosKernelImageBase;
	if(memcmp(DllName, "noskx64.exe", 12) != 0) {
		// Load the DLL Image
		SerialLog("HALT : IMPORT LIB");
		SerialLog(DllName);
		while(1);
	} // Otherwise link the kernel without changing variables
	KDebugPrint("Linking NOSKX64.EXE System");
	
	if(!ImageHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size) {
		SerialLog("NO EXPORT TABLE");
		while(1);
	}
	
	PIMAGE_EXPORT_DIRECTORY ExportDir = (PIMAGE_EXPORT_DIRECTORY)((char*)VasDll + ImageHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	

	UINT32* NamePtr = (UINT32*)((char*)VasDll + ExportDir->NamePointerRva);
	
	UINT64* ImportLookupTable = (UINT64*)((char*)VaBuffer + ImportDir->ImportLookupTable);
	UINT64* ImportAddressTable = (UINT64*)((char*)VaBuffer + ImportDir->ImportAddressTableRva);
	// SerialLog("LINKING EXECUTABLE TO DLL");

	UINT32* ExportAddressTable = (UINT32*)((char*)VasDll + ExportDir->ExportAddressTableRva);
	UINT16* ExportOrdinalTable = (UINT16*)((char*)VasDll + ExportDir->OrdinalTableRva);

	for(;*ImportLookupTable;ImportLookupTable++,ImportAddressTable++) {
		UINT64 LookupEntry = *ImportLookupTable;
		if (LookupEntry & IMAGE_ORDINAL_IMPORT_FLAG) {
			// Import By Ordinal
			UINT16 OrdinalNumber = IMAGE_ORDINAL_NUMBER(LookupEntry);
			UINT32 Rva = ExportAddressTable[OrdinalNumber];
			*ImportAddressTable = (UINT64)((char*)VasDll + Rva);
		} else {
			// Import By name
				UINT32 HintNameRva = (UINT32)IMAGE_HINT_NAME_RVA(LookupEntry);
				// HexPrint64(HintNameRva);
				PIMAGE_HINT_NAME_TABLE entry = (PIMAGE_HINT_NAME_TABLE)((char*)VaBuffer + HintNameRva);
				UINT64 ImportNameLen = strlen(entry->Name);
				// SerialLog(entry->Name);
				UINT32* n = NamePtr;
				BOOLEAN SymbolFound = FALSE;
				for (UINT32 i = 0; i < ExportDir->NumNamePointers; i++, n++) {
					char* name = (char*)((char*)VasDll + *n);

					if (strlen(name) == ImportNameLen &&
						memcmp(name, entry->Name, ImportNameLen) == 0
						) {
						// Symbol Found
						UINT16 Ordinal = ExportOrdinalTable[i];
						UINT32 Rva = ExportAddressTable[Ordinal];
						UINT64 base = (UINT64)((UINT64)VasDll + Rva);
						// SerialLog("Symbol found");
						// SerialLog(entry->Name);
						// HexPrint64(base);
						*ImportAddressTable = base;

						SymbolFound = TRUE;
						break;
					}
				}
				if (!SymbolFound) {
					SerialLog("EXPORT_SYM_NOT_FOUND");
					SerialLog(entry->Name);
					return STATUS_NOT_FOUND;
				}
		}
		
	}
	return STATUS_SUCCESS;
}