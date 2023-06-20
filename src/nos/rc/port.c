#include <nos/nos.h>
#include <rc/rc.h>
#include <ob/obutil.h>

NSTATUS PortEvt(PEPROCESS Process, UINT Evt, HANDLE Handle, UINT64 Access) {
    if(Evt == OBJECT_EVENT_OPEN) {
        // TODO : Check access and user privilege
    }
    return STATUS_SUCCESS;
}

PRCPORT RcKernelCreatePort(PDRIVER Driver, UINT64 Characteristics, char* PortName) {
    POBJECT PortObjectHeader;
    PRCPORT Port;
    if(NERROR(ObCreateObject(
        Driver->ObjectHeader,
        &PortObjectHeader,
        OBJECT_PERMANENT,
        OBJECT_RCPORT,
        PortName,
        sizeof(RCPORT),
        PortEvt
    ))) {
        KDebugPrint("RcKernelCreatePort: Failed to create port object");
        return;
    }

    Port = PortObjectHeader->Address;
    Port->Characteristics = Characteristics;
    Port->ObjectHeader = PortObjectHeader;
    Port->Driver = Driver;

    return Port;
}

// Host, can either be a process, a device connection or a driver
HANDLE RcConnectPort(HANDLE Host, char* PortName, UINT64 PortAccess) {
    if(!ObCheckHandle(Host)) return INVALID_HANDLE;
    PEPROCESS Process = KeGetCurrentProcess();
    POBJECT_REFERENCE Ref = ObiReferenceByHandle(Host);
    if(Ref->Object->ObjectType != OBJECT_PROCESS && Ref->Object->ObjectType != OBJECT_DRIVER) return INVALID_HANDLE;
    HANDLE Handle;

    // Find port object
    UINT64 _ev;
    POBJECT Obj;
    UINT NameLen = strlen(PortName);
    while((_ev = ObEnumerateObjects(Host, OBJECT_RCPORT, &Obj, NULL, _ev)) != 0) {
        if(Obj->ObjectNameLength == NameLen && memcmp(Obj->ObjectName, PortName, Obj->ObjectNameLength) == 0) {
            HANDLE Handle;
            NSTATUS Status = ObOpenHandle(Obj, Process, PortAccess, &Handle);
            if(NERROR(Status)) return Status;
            if(PortAccess & RCACCESS_ASYNC) {
                // TODO : Link a linked list between the two processes
            }
            PRCPORT Port = Obj->Address;
            _InterlockedIncrement64(&Port->NumEndpoints);
            return Handle;
        }
    }
    return INVALID_HANDLE;
}

#define RCERR(_fun, _err) {KDebugPrint("RCERROR (%s): ERR%d", _fun, _err); while(1) __halt();};

void RcDisconnect(HANDLE Port) {
    if(!ObCheckHandle(Port)) RCERR("DSC", 0);
    if(!ObLockHandle(Port)) RCERR("DSC", 1);
    POBJECT_REFERENCE Ref = ObiReferenceByHandle(Port);
    if(Ref->Object->ObjectType != OBJECT_RCPORT) {
        ObUnlockHandle(Port);
        RCERR("DSC", 2);
    }
    ObUnlockHandle(Port);
    if(!ObCloseHandle(KeGetCurrentProcess(), Port)) RCERR("DSC", 3);
}

