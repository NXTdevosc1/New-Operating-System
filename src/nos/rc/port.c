#include <nos/nos.h>
#include <rc/rc.h>
#include <ob/obutil.h>

static NSTATUS PortEvt(PEPROCESS Process, UINT Evt, HANDLE Handle, UINT64 Access) {
    if(Evt == OBJECT_EVENT_OPEN) {
        // TODO : Check access and user privilege
    }
    return STATUS_SUCCESS;
}

static NSTATUS ConnEvt(PEPROCESS Process, UINT Evt, HANDLE Handle, UINT64 Access) {
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
HANDLE KRNLAPI RcConnectPort(HANDLE Host, char* PortName, UINT64 PortAccess) {
    if(!ObCheckHandle(Host)) return INVALID_HANDLE;
    PEPROCESS Process = KeGetCurrentProcess();
    POBJECT_REFERENCE Ref = ObiReferenceByHandle(Host);
    if(Ref->Object->ObjectType != OBJECT_PROCESS && Ref->Object->ObjectType != OBJECT_DRIVER) return INVALID_HANDLE;
    HANDLE Handle;

    // Find port object
    UINT64 _ev;
    POBJECT PortObject;
    UINT NameLen = strlen(PortName);
    while((_ev = ObEnumerateObjects(Ref->Object, OBJECT_RCPORT, &PortObject, NULL, _ev)) != 0) {
        if(Obj->ObjectNameLength == NameLen && memcmp(Obj->ObjectName, PortName, Obj->ObjectNameLength) == 0) {
            // Create the connection object
            POBJECT ConnectionObject;
            UINT64 Characteristics = 0;
            HANDLE ConnectionPort;
            if(PortAccess & RCACCESS_ABSTRACT) Characteristics |= OBJECT_ABSTRACT;

            if(NERROR(ObOpenHandle(PortObject, Process, PortAccess, &ConnectionPort))) {
                KDebugPrint("RCON: ERR0");
                while(1) __halt();
            }
            if(NERROR(ObCreateObject(PortObject, &ConnectionObject, Characteristics, OBJECT_HANDLE, NULL, sizeof(RCCONNECTION), ConnEvt))) {
                KDebugPrint("RCON: ERR1");
                while(1) __halt();
            }
            PRCCONNECTION Connection = ConnectionObject->Address;
            Connection->ObjectHeader = ConnectionObject;
            Connection->Port = ConnectionPort;
            Connection->Access = PortAccess;
            
            HANDLE ConnectionHandle;
            if(NERROR(ObOpenHandle(ConnectionObject, Process, HANDLE_ALL_ACCESS, &ConnectionHandle))) {
                KDebugPrint("RCON: ERR2");
                while(1) __halt();
            }

            return ConnectionHandle;
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

