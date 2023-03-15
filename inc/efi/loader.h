/*
 * Bootloader defines
*/

#pragma once
#include <Uefi.h>


typedef struct _FRAME_BUFFER_DESCRIPTOR{
	UINT32 		HorizontalResolution;
	UINT32 		VerticalResolution;
	UINT64      Pitch;
	void* 	    BaseAddress; // for G.O.P
	UINT64 	    FbSize;
} FRAME_BUFFER_DESCRIPTOR;

typedef struct _NOS_INITDATA {
    FRAME_BUFFER_DESCRIPTOR FrameBuffer;
    EFI_MEMORY_DESCRIPTOR* MemoryMap;

} NOS_INITDATA;

#define MBR_SIGNATURE 0xAA55
#define GPT_SIGNATURE "EFI PART "
#define FSID_GPT 0xEE
#pragma pack(push, 1)

typedef struct _MASTER_BOOT_RECORD{
	char EntryPoint[440];
	UINT32 OptionnalSignature;
	UINT16 Null;
	struct {
		UINT8 Bootable; // 0x80 = active
		UINT8 ChsStart[3];
		UINT8 FileSystemId;
		UINT8 ChsEnd[3];
		UINT32 StartLba;
		UINT32 TotalSectors;
	} Parititons[4];
	UINT16 MbrSignature;
} MASTER_BOOT_RECORD;

typedef struct _GUID_PARTITION_TABLE_HEADER {
	UINT64 Signature;
	UINT32 GptRevision;
	UINT32 HeaderSize;
	UINT32 Crc32;
	UINT32 Rsv0;
	UINT64 CurrentLba;
	UINT64 SecondaryGptLba;
	UINT64 FirstUsableEntryBlock;
	UINT64 LastUsableEntryBlock;
	EFI_GUID DiskGuid;
	UINT64 GptEntryStartLba;
	UINT32 NumPartitionEntries;
	UINT32 EntrySize;
	UINT32 Crc32_OfPartitionEntryArray;
	char Rsv1[420];
} GUID_PARTITION_TABLE_HEADER;

typedef struct _GUID_PARTITION_ENTRY {
	EFI_GUID PartitionType;
	EFI_GUID UniquePartitionGuid;
	UINT64 StartingLba;
	UINT64 EndingLba;
	UINT64 Attributes;
	UINT16 PartitionName[36];
} GUID_PARTITION_ENTRY;

#pragma pack(pop)
