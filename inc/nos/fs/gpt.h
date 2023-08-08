#pragma once
#include <nosdef.h>

#define GPT_SIGNATURE 0x5452415020494645ULL

typedef struct _PARTITION_TABLE_HEADER {
    UINT64 Signature; // EFI PART
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 Crc32OfHeader;
    UINT32 rsv0;
    UINT64 CurrentLba;
    UINT64 BackupLba;
    UINT64 FirstUsableLba;
    UINT64 LastUsableLba;
    GUID   DiskGuid;
    UINT64 PartitionsLba;
    UINT32 NumPartitions;
    UINT32 PartitionEntrySize;
    UINT32 Crc32OfPartitionEntries;
} PARTITION_TABLE_HEADER;

typedef struct _GUID_PARTITION_ENTRY {
    GUID PartitionTypeGuid;
    GUID UniquePartitionGuid;
    UINT64 FirstLba;
    UINT64 LastLba;
    UINT64 PartitionAttributes;
    UINT16 PartitionName[36];
} GUID_PARTITION_ENTRY;

// BASIC DATA PARTITION EBD0A0A2-B9E5-4433-87C0-68B6B72699C7.[2]
#define BASIC_DATA_PARTITION_GUID {0xEBD0A0A2,0xB9E5,0x4433, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}
#define EFI_SYSTEM_PARTITION_GUID {0xC12A7328, 0xF81F, 0x11D2, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}

#define GPT_ATTR_READONLY ((UINT64)1 << 60)
#define GPT_ATTR_SHADOWCOPY ((UINT64)1 << 61)
#define GPT_ATTR_HIDDEN ((UINT64)1 << 62)
#define GPT_ATTR_NODRIVELETTER ((UINT64)1 << 63)
