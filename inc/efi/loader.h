/*
 * Bootloader defines
*/

#pragma once
#include <Uefi.h>


typedef struct _FRAME_BUFFER_DESCRIPTOR{
	UINT32 		HorizontalResolution;
	UINT32 		VerticalResolution;
	UINT64      Pitch;
	UINT32* 	BaseAddress; // for G.O.P
	UINT64 	    FbSize;
} FRAME_BUFFER_DESCRIPTOR;

typedef struct _NOS_INITDATA {
    FRAME_BUFFER_DESCRIPTOR FrameBuffer;
    EFI_MEMORY_DESCRIPTOR* MemoryMap;

} NOS_INITDATA;