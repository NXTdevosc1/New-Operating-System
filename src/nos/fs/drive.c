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