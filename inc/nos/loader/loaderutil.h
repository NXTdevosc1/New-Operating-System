#pragma once
#include <nos/nos.h>
#include <nos/loader/pe.h>


#define LoaderRead(_base) (((char*)&base + 8) > ImageSize ? (return STATUS_OUT_OF_BOUNDS) : (_base))

#define LoaderGetNewHeaderOffset(DosHeader) ((PIMAGE_NT_HEADERS)((char*)DosHeader + ((PIMAGE_DOS_HEADER)DosHeader)->e_lfanew))


NSTATUS LoaderRelocateImage(
    IMAGE_NT_HEADERS* Image,
    void* ImageFile,
    void* VaBuffer,
    void* NewBaseAddress
);

NSTATUS LoaderImportLibrary(
	IMAGE_NT_HEADERS* Image,
	PIMAGE_IMPORT_DIRECTORY ImportDir,
	void* VaBuffer,
	char* DllName,
    POBJECT ParentObject
);