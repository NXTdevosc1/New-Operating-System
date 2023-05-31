#include <nos/nos.h>
#include <nos/loader/loaderutil.h>
static char __fmtloader[100] = {0};
#define HexPrint64(Num) {_ui64toa(Num, __fmtloader, 0x10); SerialLog(__fmtloader);} 


NSTATUS KRNLAPI KeLoadImage(
    void* ImageBuffer,
    UINT64 ImageSize,
    BOOLEAN OperatingMode,
    // If KERNEL_MODE then _Out will hold the value of the entry point address
    // otherwise it will return a pointer to the process
    // if Image type is a DLL, then the caller must set the parent process address in _Out
    void** _Out
) {
    if(ImageSize < 0x200) return STATUS_INVALID_FORMAT;
    NSTATUS Status;
    PIMAGE_NT_HEADERS Header = LoaderGetNewHeaderOffset(ImageBuffer);
    // We require a relocatable 64 Bit Image
    if(Header->Signature != IMAGE_NT_SIGNATURE ||
    Header->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
    Header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
    Header->OptionalHeader.Subsystem > 0xFF ||
    !(Subsystems[Header->OptionalHeader.Subsystem].Flags & 1) ||
    Subsystems[Header->OptionalHeader.Subsystem].OperatingMode != OperatingMode ||
    (Header->OptionalHeader.DllCharacteristics & (IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)) != (IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)
    ) return STATUS_INVALID_FORMAT;


    PIMAGE_SECTION_HEADER Sections = (PIMAGE_SECTION_HEADER)((char*)&Header->OptionalHeader + Header->FileHeader.SizeOfOptionalHeader);
    UINT64 AddressSpaceSize = 0;
    // Check sections and determine address space size
    for(int i = 0;i<Header->FileHeader.NumberOfSections;i++) {
        if(Sections[i].PointerToRawData + Sections[i].SizeOfRawData > ImageSize) {
            return STATUS_INVALID_FORMAT;
        }
        if(Sections[i].Characteristics & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
        (Sections[i].VirtualAddress + Sections[i].VirtualSize) > AddressSpaceSize
        ) {
            AddressSpaceSize = Sections[i].VirtualAddress + Sections[i].VirtualSize;
        }
        SerialLog("Section:");
        SerialLog(Sections[i].Name);
        HexPrint64(Sections[i].VirtualAddress);
        HexPrint64(Sections[i].PointerToRawData);
        HexPrint64(Sections[i].VirtualSize);

    }
    AddressSpaceSize = AlignForward(AddressSpaceSize, 0x1000);

    SerialLog("ADDR_SPACE_SIZE :");
    HexPrint64(AddressSpaceSize);

    UINT AllocateFlags = PAGE_WRITE_ACCESS;
    if(OperatingMode == USER_MODE) AllocateFlags |= PAGE_USER;

    // TODO : if user mode, use the created process
    void* VaBuffer = MmAllocateMemory(KernelProcess, AddressSpaceSize >> 12, AllocateFlags, 0);
    if(!VaBuffer) return STATUS_OUT_OF_MEMORY;
    // Copy sections content

    for(int i = 0;i<Header->FileHeader.NumberOfSections;i++) {
        if(Sections[i].Characteristics & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA)) {
            UINT64 sz = Sections[i].SizeOfRawData;
            if(Sections[i].VirtualSize < sz) sz = Sections[i].VirtualSize;
            memcpy((char*)VaBuffer + Sections[i].VirtualAddress, (char*)ImageBuffer + Sections[i].PointerToRawData, Sections[i].SizeOfRawData);
        }
        if(Sections[i].Characteristics & (IMAGE_SCN_CNT_UNINITIALIZED_DATA)) {
            memset((char*)VaBuffer + Sections[i].VirtualAddress + Sections[i].SizeOfRawData, 0, Sections[i].VirtualSize - Sections[i].SizeOfRawData);
        }
    }
    SerialLog("Sections copied.");

    // Relocate if needed
    
    if(Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
        SerialLog("Relocating...");
        Status = LoaderRelocateImage(Header, ImageBuffer, VaBuffer, VaBuffer);
        if(NERROR(Status)) return Status;
    }

    // Import if needed

    if(Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {
        SerialLog("IMPORTING...");
        PIMAGE_IMPORT_DIRECTORY ImportDir = (PIMAGE_IMPORT_DIRECTORY)((char*)VaBuffer + Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        for(;ImportDir->NameRva;ImportDir++) {
            char* DllName = (char*)VaBuffer + ImportDir->NameRva;
            SerialLog(DllName);
            LoaderImportLibrary(Header, ImportDir, VaBuffer, DllName);
        }
        
    }

    // Export if needed
    if(*_Out) {
        if(!Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size) {
            SerialLog("Imported DLL does not contain EXPORT Section.");
            return STATUS_NOT_FOUND;
        }
        SerialLog("EXPORTING...");
    }

    if(OperatingMode == KERNEL_MODE) {
        *_Out = (char*)VaBuffer + Header->OptionalHeader.AddressOfEntryPoint;
    }


    return STATUS_SUCCESS;
}