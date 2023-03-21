/*
 * Bootloader defines
*/

#pragma once
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

#define PE_SECTION_CODE 0x20
#define PE_SECTION_INITIALIZED_DATA 0x40
#define PE_SECTION_UNINITIALIZED_DATA 0x80

typedef struct _FRAME_BUFFER_DESCRIPTOR{
	UINT32 		HorizontalResolution;
	UINT32 		VerticalResolution;
	UINT64      Pitch;
	void* 	    BaseAddress; // for G.O.P
	UINT64 	    FbSize;
} FRAME_BUFFER_DESCRIPTOR;

// Memory Manager Attributes
#define MM_DESCRIPTOR_ALLOCATED 1
typedef struct _NOS_MEMORY_DESCRIPTOR {
    UINT32 Attributes;
    void* PhysicalAddress;
    UINT64 NumPages;
} NOS_MEMORY_DESCRIPTOR;
typedef struct _NOS_MEMORY_LINKED_LIST NOS_MEMORY_LINKED_LIST;
typedef struct _NOS_MEMORY_LINKED_LIST {
    UINT64 Full; // Bitmap indicating full slot groups
    struct {
        UINT64 Present; // Bitmap indicating present heaps
        NOS_MEMORY_DESCRIPTOR MemoryDescriptors[64];
    } Groups[0x40];
    NOS_MEMORY_LINKED_LIST* Next;
} NOS_MEMORY_LINKED_LIST;

typedef enum _PAGE_MAP_FLAGS {
    PM_WRITEACCESS = 1,
    PM_GLOBAL = 2,
    PM_LARGE_PAGES = 4
} PAGE_MAP_FLAGS;

typedef struct _NOS_INITDATA {
    // Nos Image Data
    void* NosKernelImageBase;
    void* NosPhysicalBase;
    UINT64 NosKernelImageSize;

    // EFI Frame Buffer
    FRAME_BUFFER_DESCRIPTOR FrameBuffer;
    // EFI Memory Map
    UINT64 MemoryCount;
    UINT64 MemoryDescriptorSize;
    EFI_MEMORY_DESCRIPTOR* MemoryMap;
    // System Startup Drive Info

    // NOS Kernel Memory Map
    NOS_MEMORY_LINKED_LIST* NosMemoryMap;
    UINT64 AllocatedPagesCount;
    UINT64 FreePagesCount;

} NOS_INITDATA;


typedef struct _IMAGE_IMPORT_ADDRESS_TABLE {
    UINT64 ImportAddress;
    UINT32 ForwarderRva;
} IMAGE_IMPORT_ADDRESS_TABLE, *PIMAGE_IMPORT_ADDRESS_TABLE;

// We support only PE32+ 64 bit image files
typedef struct {
    UINT16 magic; // must be 0x20b
    UINT8 MajorLinkerVersion;
    UINT8 MinorLinkerVersion;
    UINT32 SizeofCode;
    UINT32 SizeofInitializedData;
    UINT32 SizeofUninitializedData; // .bss ...
    UINT32 EntryPointAddr;
    UINT32 BaseOfCode;
} PE_OPTIONAL_HEADER;

typedef struct {
    UINT64 ImageBase;
    UINT32 SectionAlignment; // greater than or equal to file alignement
    UINT32 FileAlignment;
    UINT16 MajorOsVersion;
    UINT16 MinorOsVersion;
    UINT16 MajorImageVersion;
    UINT16 MinorImageVersion;
    UINT16 MajorSubsystemVersion;
    UINT16 MinorSubsystemVersion;
    UINT32 Win32VersionValue; // reserved must be 0
    UINT32 SizeofImage;
    UINT32 SizeofHeaders;
    UINT32 CheckSum;
    UINT16 Subsystem; // this is used to load image ex. from main or wWinMain or driver entry ...
    UINT16 DllCharacteristics;
    UINT64 StackReserve;
    UINT64 StackCommit;
    UINT64 HeapReserve;
    UINT64 HeapCommit;
    UINT32 LoaderFlags; // reserved must be 0
    UINT32 NumDataDirectories;
} PE_WINDOWS_SPECIFIC_FIELDS;
typedef struct {
    char name[8];
    UINT32 VirtualSize;
    UINT32 VirtualAddress;
    UINT32 SizeofRawData; // if less than virtual size reminder of file align is filled with 0
    UINT32 PtrToRawData; // for object file it should be 4 byte aligned for best performance
    UINT32 PtrToRelocations; // should be 0 for executables
    UINT32 PtrToLineNumbers;
    UINT16 NumRelocations; // set to 0 for executables
    UINT16 NumLineNumbers;
    UINT32 Characteristics; // section flags
} PE_SECTION_TABLE;

typedef struct _IMAGE_DATA_DIRECTORY {
    UINT32   VirtualAddress;
    UINT32   Size;
} IMAGE_DATA_DIRECTORY, * PIMAGE_DATA_DIRECTORY;

typedef struct _OPTIONNAL_HEADER_DATA_DIRECTORIES {
    IMAGE_DATA_DIRECTORY ExportTable;
    IMAGE_DATA_DIRECTORY ImportTable;
    IMAGE_DATA_DIRECTORY ResourceTable;
    IMAGE_DATA_DIRECTORY ExceptionTable;
    IMAGE_DATA_DIRECTORY CertificateTable;
    IMAGE_DATA_DIRECTORY BaseRelocationTable;
    IMAGE_DATA_DIRECTORY Debug;
    IMAGE_DATA_DIRECTORY Architecture;
    IMAGE_DATA_DIRECTORY GlobalPtr;
    IMAGE_DATA_DIRECTORY ThreadLocalStorageTable;
    IMAGE_DATA_DIRECTORY LoadConfigTable;
    IMAGE_DATA_DIRECTORY BoundImport;
    IMAGE_DATA_DIRECTORY ImportAddressTable;
    IMAGE_DATA_DIRECTORY DelayImportDescriptor;
    IMAGE_DATA_DIRECTORY ClrRuntimeHeader;
    IMAGE_DATA_DIRECTORY Reserved;
} OPTIONNAL_HEADER_DATA_DIRECTORIES;

typedef struct {
    char Signature[4]; // PE\0\0
    UINT16 MachineType; // must be AMD64, for x86_64 and AMD64
    UINT16 NumSections;
    UINT32 TimeDateStamp;
    UINT32 SymbolTableOffset;
    UINT32 NumSymbols;
    UINT16 SizeofOptionnalHeader;
    UINT16 Characteristics;
    PE_OPTIONAL_HEADER OptionnalHeader; // not optional because it needs to be a loadable image
    PE_WINDOWS_SPECIFIC_FIELDS ThirdHeader;
    OPTIONNAL_HEADER_DATA_DIRECTORIES OptionnalDataDirectories;
} PE_IMAGE_HDR;



typedef struct _IMAGE_IMPORT_DIRECTORY {
    UINT32 ImportLookupTable;
    UINT32 TimeDataStamp;
    UINT32 ForwarderChain;
    UINT32 NameRva;
    UINT32 ImportAddressTableRva;
} IMAGE_IMPORT_DIRECTORY, *PIMAGE_IMPORT_DIRECTORY;

enum IMAGE_BASE_RELOCATION_TYPE{
    IMAGE_REL_BASED_ABSOLUTE = 0,
    IMAGE_REL_BASED_HIGH = 1,
    IMAGE_REL_BASED_LOW = 2,
    IMAGE_REL_BASED_HIGHLOW = 3,
    IMAGE_REL_BASED_HIGHADJ = 4,
    IMAGE_REL_BASED_MIPS_JMPADDR = 5,
    IMAGE_REL_BASED_DIR64 = 10
};

typedef struct _IMAGE_BASE_RELOCATION_ENTRY {
    UINT16 Offset : 12; // 4 bits Type, 12 bits Offset
    UINT16 Type : 4;
} IMAGE_BASE_RELOCATION_ENTRY, * PIMAGE_BASE_RELOCATION_ENTRY;

typedef struct _IMAGE_BASE_RELOCATION_BLOCK {
    UINT32 PageRva; // Page Relative Virtual Address
    UINT32 BlockSize; // total num bytes in base relocation block
    IMAGE_BASE_RELOCATION_ENTRY RelocationEntries[];
} IMAGE_BASE_RELOCATION_BLOCK, *PIMAGE_BASE_RELOCATION_BLOCK;

typedef struct _IMAGE_EXPORT_DIRECTORY {
    UINT32 ExportFlags; // reserved must be 0
    UINT32 TimeDateStamp;
    UINT16 MajorVersion;
    UINT16 MinorVersion;
    UINT32 NameRva; // Dll Name
    UINT32 OrdinalBase;
    UINT32 NumAddressTableEntries;
    UINT32 NumNamePointers; // = NumOrdinalEntries
    UINT32 ExportAddressTableRva;
    UINT32 NamePointerRva;
    UINT32 OrdinalTableRva;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

typedef struct _IMAGE_EXPORT_ADDRESS_TABLE {
    UINT32 ExportRva;
    UINT32 ForwarderRva; // this one gives the dllname.exportname or export name with ordinal number
} IMAGE_EXPORT_ADDRESS_TABLE, *PIMAGE_EXPORT_ADDRESS_TABLE;

typedef struct _IMAGE_HINT_NAME_TABLE {
    UINT16 Hint;
    char Name[];
} IMAGE_HINT_NAME_TABLE, *PIMAGE_HINT_NAME_TABLE;

#define IMAGE_ORDINAL_IMPORT_FLAG 0x8000000000000000
#define IMAGE_ORDINAL_NUMBER(LookupEntry) (LookupEntry & 0xffff)
#define IMAGE_HINT_NAME_RVA(LookupEntry) (LookupEntry & (0x7fffffff));


typedef void __attribute__((noreturn)) (*NOS_ENTRY_POINT)();

// util.c
BOOLEAN BlNullGuid(EFI_GUID Guid);
BOOLEAN isMemEqual(void* a, void* b, UINT64 Count);
void CopyAlignedMemory(void* _dest, void* _src, UINT64 NumBytes);
void ZeroAlignedMemory(void* _dest, UINT64 NumBytes);
const char* ToStringUint64(UINT64 value);
const char* ToHexStringUint64(UINT64 value);

// bootgfx.c
void BlInitBootGraphics();

// load.c
BOOLEAN BlLoadImage(void* Buffer, PE_IMAGE_HDR** HdrStart, void** VirtualAddressSpace, UINT64* VasSize, void* BaseAddress);

// map.c
void BlMapToSystemSpace(void* Buffer, UINT64 Num2mbPages);
extern NOS_INITDATA NosInitData;
extern void* BlGetCurrentPageTable();
void BlAllocateMemoryDescriptor(EFI_PHYSICAL_ADDRESS Address, UINT64 NumPages, BOOLEAN Allocated);
void* BlAllocateOnePage();
void QemuWriteSerialMessage(const char* Message);


void BlInitPageTable();
void BlMapMemory(
    void* VirtualAddress,
    void* PhysicalAddress,
    UINT64 Count,
    UINT64 Flags);
void BlInitSystemHeap(UINTN NumLargePages);
void* BlAllocateSystemHeap(UINTN NumPages);
#define Convert2MBPages(NumBytes) ((NumBytes & 0x1FFFFF) ? ((NumBytes >> 21) + 1) : (NumBytes >> 21))