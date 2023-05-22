#include <nos/nos.h>
#include <nos/loader/loaderutil.h>

char __fmtloader[100];

#define HexPrint64(Num) {_ui64toa(Num, __fmtloader, 0x10); SerialLog(__fmtloader);} 

NSTATUS KRNLAPI KeLoadImage(
    void* ImageBuffer,
    UINT64 ImageSize,
    // Process is only returned if it is a usermode executable
    // if Image type is a DLL, then Process contains the parent executable
    PEPROCESS* Process
) {
    PIMAGE_NT_HEADERS Header = LoaderGetNewHeaderOffset(ImageBuffer);

    if(Header->Signature != IMAGE_NT_SIGNATURE ||
    Header->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
    Header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
    Header->OptionalHeader.Subsystem > 0xFF ||
    !(Subsystems[Header->OptionalHeader.Subsystem].Flags & 1)
    ) return STATUS_INVALID_FORMAT;

    

    return STATUS_SUCCESS;
}