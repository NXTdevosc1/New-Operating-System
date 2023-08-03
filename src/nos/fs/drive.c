#include <nos/nos.h>
#include <nos/fs/fs.h>

NSTATUS DriveEvt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access) {
    return STATUS_SUCCESS;
}

PDRIVE KRNLAPI KeCreateDrive(
    IN PREALLOCATED DRIVE_IDENTIFICATION_DATA* DriveId,
    IN PDRIVEIO DriveIo,
    IN void* Context
) {
    POBJECT Obj;
    PDRIVE Drive;
    if(NERROR(ObCreateObject(
        NULL, &Obj, OBJECT_PERMANENT,
        OBJECT_DRIVE, NULL, sizeof(DRIVE), DriveEvt
    ))) {
        KDebugPrint("CREATE_DRIVE BUG0");
        while(1) __halt();
    }
    Drive = Obj->Address;
    Drive->Obj = Obj;
    Drive->DriveId = DriveId;

    Drive->Io.Read = DriveIo->Read;
    Drive->Io.Write = DriveIo->Write;
    Drive->Io.Allocate = DriveIo->Allocate;
    Drive->Io.Free = DriveIo->Free;
    Drive->Context = Context;

    Drive->PartitionTableStatus = KiReadPartitionTable(Drive);

    return Drive;
}

BOOLEAN KRNLAPI KeReadDrive(
    PDRIVE Drive,
    UINT64 Sector,
    UINT64 Count,
    void* Buffer
) {
    if(Sector + Count >= Drive->DriveId->NumSectors) return FALSE;

    NSTATUS Status = Drive->Io.Read(
        Drive->Context,
        Sector,
        Count,
        Buffer
    );
    if(NERROR(Status)) KeRaiseException(Status);

    return TRUE;
}

BOOLEAN KRNLAPI KeWriteDrive(
    PDRIVE Drive,
    UINT64 Sector,
    UINT64 Count,
    void* Buffer
) {
    if(Sector + Count >= Drive->DriveId->NumSectors) return FALSE;
    
    NSTATUS Status = Drive->Io.Write(
        Drive->Context,
        Sector,
        Count,
        Buffer
    );
    if(NERROR(Status)) KeRaiseException(Status);

    return TRUE;
}

PVOID KRNLAPI KeAllocateDriveBuffer(
    PDRIVE Drive,
    UINT64 SizeInSectors
) {
    if(!SizeInSectors) KeRaiseException(STATUS_INVALID_PARAMETER);

    return Drive->Io.Allocate(Drive->Context, SizeInSectors);
}

void KRNLAPI KeFreeDriveBuffer(
    PDRIVE Drive,
    void* Buffer,
    UINT64 SizeInSectors
) {
    if(!SizeInSectors) KeRaiseException(STATUS_INVALID_PARAMETER);
    Drive->Io.Free(Drive->Context, Buffer, SizeInSectors);
}