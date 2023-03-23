/*
 * NOS Kernel Header
*/

#pragma once

#include <nostypes.h>
#include <efi.h>
#include <intrin.h>
#include <serial.h>
#ifdef DEBUG
#define SerialLog(Msg) {SerialWrite(Msg);} 
#else
#define SerialLog(Msg)
#endif
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
    EFI_RUNTIME_SERVICES* EfiRuntimeServices;
    // NOS Kernel Memory Map
    NOS_MEMORY_LINKED_LIST* NosMemoryMap;
    UINT64 AllocatedPagesCount;
    UINT64 FreePagesCount;

} NOS_INITDATA;

extern NOS_INITDATA* NosInitData;