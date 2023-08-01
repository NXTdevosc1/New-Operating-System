#include <nos/nos.h>
#include <nos/fs/fs.h>


NSTATUS PartEvt(
    PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access
) {
    return STATUS_SUCCESS;
}


PPARTITION KRNLAPI KeCreatePartition(
    IN PDEVICE Drive,
    IN GUID* PartitionGuid,
    UINT FsId,
    UINT16* PartitionLabel
) {
    POBJECT Obj;
    PPARTITION Partition;
    if(NERROR(ObCreateObject(
        Drive->ObjectDescriptor,
        &Obj, OBJECT_PERMANENT, OBJECT_PARTITION,
        NULL, sizeof(PARTITION), PartEvt
    ))) {
        KDebugPrint("CREATE_PARTITION BUG0");
        while(1) __halt();
    }
    Partition = Obj->Address;
    Partition->Obj = Obj;
    Partition->Drive = Drive;

    // TODO : Generate random partition guid
    memcpy(&Partition->PartitionGuid, PartitionGuid, sizeof(GUID));

    Partition->FsId = FsId;
    Partition->LengthPartitionLabel = wcslen(PartitionLabel);
    memcpy(Partition->PartitionLabel, PartitionLabel, Partition->LengthPartitionLabel << 1);
    Partition->MountIndex = -1;

    

    return Partition;
}

SPINLOCK ManageDriveLetters = 0;

volatile UINT32 _KiAssignedDriveLetters = 0;
PPARTITION MountedPartitions[26] = {0}; // From A to Z
BOOLEAN KRNLAPI KeMountPartition(
    PPARTITION Partition,
    UCHAR DriveLetter // From A to Z (Uppercase) 0 for a random letter
) {

    if(DriveLetter && 
    (DriveLetter < 'A' || DriveLetter > 'Z')) return FALSE;
    ULONG MountIndex = 0;
    UINT64 Cpf;
    if(!DriveLetter) {
        Cpf = ExAcquireSpinLock(&ManageDriveLetters);
        _BitScanForward(&MountIndex, ~_KiAssignedDriveLetters);
        if(MountIndex > 25) {
            ExReleaseSpinLock(&ManageDriveLetters, Cpf);
            return FALSE; // No free partitions
        }
        _bittestandset(&_KiAssignedDriveLetters, MountIndex);
    } else {
        MountIndex = DriveLetter - 'A';
        Cpf = ExAcquireSpinLock(&ManageDriveLetters);
        if(_bittestandset(&_KiAssignedDriveLetters, MountIndex)) {
            ExReleaseSpinLock(&ManageDriveLetters, Cpf);
            return FALSE; // Drive letter already in use
        }
    }

    MountedPartitions[MountIndex] = Partition;


    ExReleaseSpinLock(&ManageDriveLetters, Cpf);

    return TRUE;
}

BOOLEAN KRNLAPI KeUnmountPartition(
    UCHAR DriveLetter
) {
    if(DriveLetter < 'A' || DriveLetter > 'Z') return FALSE;
    ULONG MountIndex = DriveLetter - 'A';
    UINT64 cpf = ExAcquireSpinLock(&ManageDriveLetters);
    BOOLEAN exists = _bittestandreset(&_KiAssignedDriveLetters, MountIndex);
    if(exists) {
        MountedPartitions[MountIndex]->Mounted = FALSE;
        MountedPartitions[MountIndex]->MountIndex = -1;

        MountedPartitions[MountIndex] = NULL;
    }

    ExReleaseSpinLock(&ManageDriveLetters, cpf);

    return exists;
}