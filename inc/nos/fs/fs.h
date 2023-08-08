#include <kfs.h>



typedef struct _PARTITION {
    POBJECT Obj;
    PDEVICE Drive;
    GUID PartitionGuid;
    UINT16 FileSystemLabel[33];
    UINT8 LengthFsLabel;
    UINT16 PartitionLabel[PARTITION_NAME_LENGTH_MAX + 1];
    UINT8 LengthPartitionLabel;
    BOOLEAN Mounted;
    UINT8 MountIndex;
} PARTITION, *PPARTITION;


// Files need to be accessed fast
typedef struct _FILE {
    PPARTITION Partition;
    // Path inside the partition
    UINT16 Path[FILE_PATH_LENGTH_MAX];
    UINT8 LengthPath;
} FILE, *PFILE;

NSTATUS KiReadPartitionTable(PDRIVE Drive);