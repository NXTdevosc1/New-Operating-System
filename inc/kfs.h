#pragma once
#include <nosdef.h>

#define DRIVE_NAME_LENGTH_MAX 255
#define PARTITION_NAME_LENGTH_MAX DRIVE_NAME_LENGTH_MAX
#define FILE_PATH_LENGTH_MAX DRIVE_NAME_LENGTH_MAX

// File management



typedef enum _FILE_OPEN_TYPE {
    FILE_OPEN_EXISTS,
    FILE_OPEN_CREATE,
    FILE_OPEN_DELETE // returns 0 on success, INVALID_HANDLE on failure
} FILE_OPEN_TYPE;

// File desired access
#define FILE_ACCESS_READ 1
#define FILE_ACCESS_WRITE 2
#define FILE_ACCESS_EDIT 4 // Edit file information

// Drive management

typedef struct _DRIVE_IDENTIFICATION_DATA {
    UINT16 SectorSize; // In bytes (Should be 512 byte aligned)
    UINT64 NumSectors;
    PDEVICE Device;
    UINT16 Name[DRIVE_NAME_LENGTH_MAX + 1]; // MAX 255 Characters
    
} DRIVE_IDENTIFICATION_DATA;
// Any Status other than SUCCESS will trigger a system crash
typedef NSTATUS (__fastcall* DRIVE_READ)(void* Context, UINT64 Sector, UINT64 Count, PVOID Buffer);
typedef NSTATUS (__fastcall* DRIVE_WRITE)(void* Context, UINT64 Sector, UINT64 Count, PVOID Buffer);


// Allocates a buffer for read/write operations so that the address will be compatible with the drive
typedef PVOID (__fastcall* DRIVE_BUFFER_ALLOCATE)(void* Context, UINT64 SizeInSectors);
typedef void (__fastcall* DRIVE_BUFFER_FREE)(void* Context, void* Buffer, UINT64 SizeInSectors);

typedef struct _DRIVEIO {
    DRIVE_READ Read;
    DRIVE_WRITE Write;
    DRIVE_BUFFER_ALLOCATE Allocate;
    DRIVE_BUFFER_FREE Free;
} DRIVEIO, *PDRIVEIO;

typedef struct _DRIVE {
    POBJECT Obj;
    DRIVE_IDENTIFICATION_DATA* DriveId;
    NSTATUS PartitionTableStatus;
    UINT NumPartitions;
    DRIVEIO Io;
    void* Context;
} DRIVE, *PDRIVE;

typedef struct _DRIVE DRIVE, *PDRIVE;

PDRIVE KRNLAPI KeCreateDrive(
    IN PREALLOCATED DRIVE_IDENTIFICATION_DATA* DriveId,
    IN PDRIVEIO DriveIo,
    IN OPT void* Context
);

BOOLEAN KRNLAPI KeReadDrive(
    PDRIVE Drive,
    UINT64 Sector,
    UINT64 Count,
    void* Buffer
);

BOOLEAN KRNLAPI KeWriteDrive(
    PDRIVE Drive,
    UINT64 Sector,
    UINT64 Count,
    void* Buffer
);

PVOID KRNLAPI KeAllocateDriveBuffer(
    PDRIVE Drive,
    UINT64 SizeInSectors
);

void KRNLAPI KeFreeDriveBuffer(
    PDRIVE Drive,
    void* Buffer,
    UINT64 SizeInSectors
);

// Partition management

typedef struct _SYSTEM_PARTITION_ADD_CONTEXT {
    PDRIVE Drive;
    BOOLEAN GuidPartition;
    GUID PartitionInstanceGuid;
    UINT64 PartitionAttributes;
    UINT64 StartLba;
    UINT64 EndLba;
    void* EventDesc;
} SYSTEM_PARTITION_ADD_CONTEXT;

typedef struct _PARTITION PARTITION, *PPARTITION;



PPARTITION KRNLAPI KeCreatePartition(
    IN PDEVICE Drive,
    IN GUID* PartitionGuid,
    UINT64 PartitionAttributes,
    UINT16* FileSystemLabel, // Max 32 Characters
    UINT16* PartitionLabel // Max 64 Characters
);

BOOLEAN KRNLAPI KeMountPartition(
    PPARTITION Partition,
    UCHAR DriveLetter // From A to Z (Uppercase) 0 for a random letter
);

BOOLEAN KRNLAPI KeUnmountPartition(
    UCHAR DriveLetter
);

