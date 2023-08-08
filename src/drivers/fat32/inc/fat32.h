#pragma once
#include <ddk.h>
#include <kfs.h>

#pragma pack(push, 1)

typedef struct _BIOS_PARAMETER_BLOCK {
    UINT16 SectorSize; // In Bytes (Must be 512)
    UINT8 ClusterSize; // Num Sectors in a cluster
    UINT16 ReservedAreaSize; // In our O.S This area contains the BootSector, Boot Manager and FS Descriptors
    UINT8 FatCount; // Usually 2 (We do accept only 2 or 1)
    UINT16 Ignored0; // MaxRootDirEntries (Reserved in FAT32)
    UINT16 Ignored1;
    UINT8 MediaDescriptor;
    UINT16 Ignored2;
} BIOS_PARAMETER_BLOCK;

typedef struct _DOS3_PARAMBLOCK {
    UINT16 SectorsPerTrack;
    UINT16 NumHeads;
    UINT32 HiddenSectors; // Sectors that preceeds the partition, or partition base in LBA
    UINT32 TotalSectors;
} DOS3_PARAMBLOCK;

typedef struct _FAT32_EBIOS_PARAMBLOCK {
    UINT32 SectorsPerFat;
    UINT16 DriveDescription;
    UINT16 Version; // usually set to 0
    UINT32 RootDirStart; // Usually 2
    UINT16 FsInfoLba; // LBA of FS_INFO - HiddenSectors
    UINT16 BootSectorCopyLba; // LBA of BS_COPY - HiddenSectors
    UINT8 Reserved[12];
    UINT8 PhysicalDriveNumber;
    UINT8 Unused;
    UINT8 ExtendedBootSignature; // 0x29
    UINT32 VolumeId;
    UINT8 VolumeLabel[11];
    UINT8 FsLabel[8];
} FAT32_EBIOS_PARAMBLOCK;


typedef enum _FAT_FILE_ATTRIBUTES {
    FAT_READ_ONLY = 1,
    FAT_HIDDEN = 2,
    FAT_SYSTEM = 4,
    FAT_VOLUME_LABEL = 8,
    FAT_DIRECTORY = 0x10,
    FAT_ARCHIVE = 0x20,
    FAT_DEVICE = 0x40
} FAT_FILE_ATTRIBUTES;

typedef struct _FAT32_BOOTSECTOR {
    char Jmp[3];
    char OemName[8];
    BIOS_PARAMETER_BLOCK BiosParamBlock;
    DOS3_PARAMBLOCK DosParamBlock;
    FAT32_EBIOS_PARAMBLOCK ExtendedBiosParamBlock;
    UINT8 BootCode[420]; // in our operating system this is not used
    UINT16 BootSignature; // 0xAA55
} FAT32_BOOTSECTOR;

typedef struct _FAT32_FILE_SYSTEM_INFORMATION {
    UINT8 Signature0[4]; // RRaA
    UINT8 Reserved0[480];
    UINT8 Signature1[4]; // rrAa
    UINT32 FreeClusters; // Num free data clusters on volume
    UINT32 RecentlyAllocatedCluster;
    UINT8 Reserved1[12];
    UINT16 NullSignature;
    UINT16 BootSignature; // 0xAA55
    unsigned char Reserved2[510];
    UINT16 BootSignature2;
} FAT32_FILE_SYSTEM_INFORMATION;

typedef struct _FAT32_FILE_ENTRY {
    UINT8 ShortFileName[8]; // Byte0 may contain flags, padded with spaces
    UINT8 ShortFileExtension[3]; // Padded with spaces
    UINT8 FileAttributes;
    UINT8 UserAttributes;
    UINT8 DeletedFileFirstChar;
    struct {
        UINT16 SecondsDivBy2 : 5;
        UINT16 Minutes : 6;
        UINT16 Hours : 5;
    } Time;
    struct {
        UINT16 Day : 5;
        UINT16 Month : 4;
        UINT16 YearFrom1980 : 7;
    } Date;
    UINT8 CreatorUserId; // OS Specific
    UINT8 CreatorGroupId; // OS Specific
    UINT16 FirstClusterHigh;
    struct {
        UINT16 SecondsDivBy2 : 5;
        UINT16 Minutes : 6;
        UINT16 Hours : 5;
    } LastModifiedTime;
    struct {
        UINT16 Day : 5;
        UINT16 Month : 4;
        UINT16 YearFrom1980 : 7;
    } LastModifiedDate;
    UINT16 FirstClusterLow;
    UINT32 FileSize;
} FAT32_FILE_ENTRY;

/*
FOR_LFN : 6 FIRST CHARS + ~ + Number

DRIVER : ~LFN_!EN
*/

#define FAT32_LFN_MARK "~LFN_!EN   "

#define FAT32_ENDOF_CHAIN 0x0FFFFFFF

typedef struct _FAT32_LONG_FILE_NAME_ENTRY {
    UINT8 SequenceNumber; // bit 6 = last logical, 0xE5 = Deleted
    UINT16 Name0[5];
    UINT8 Attributes; // Always 0x0F
    UINT8 Ignored0;
    UINT8 LfnChecksum; // Checksum of dos filename
    UINT16 Name1[6];
    UINT16 Ignored1;
    UINT16 Name2[2];
} FAT32_LONG_FILE_NAME_ENTRY;

#pragma pack(pop)


/*
 * TODO : File management should be synchronous accross a single directory
   Modifying fat sectors should be asynchrounous and should accessing different directories be

  Currently everything is synchronous accross all the file system

  TODO : Implement file system cache
*/
typedef struct {
    PDRIVE Drive;
    FAT32_BOOTSECTOR* BootSector;
    UINT8 NumFats;
    UINT64 FatLba, RootDirLba;

    UINT8 ClusterSize;

    UINT64 LastReadFatLba; // TODO : Implement File System Cache
    UINT32* LastReadFatBuffer;
} FAT32_INSTANCE;

#include <intrin.h>


NSTATUS Fat32PartitionAddEvt(SYSTEM_PARTITION_ADD_CONTEXT* Context);

UINT32 FatReadCluster(FAT32_INSTANCE* fs, UINT Cluster, void* Buffer);

#define FatAllocateClusters(fs, NumClusters) KeAllocateDriveBuffer(fs->Drive, NumClusters * fs->ClusterSize)


BOOLEAN FatReadDirectory(
    FAT32_INSTANCE* fs,
    UINT Cluster
);