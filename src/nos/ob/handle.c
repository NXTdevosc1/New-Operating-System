#include <nos/ob/obutil.h>

NSTATUS KRNLAPI ObOpenHandle(
    IN POBJECT Object,
    IN PEPROCESS Process,
    IN UINT64 Access,
    IN HANDLE* Handle
) {
    if(!Process) Process = KernelProcess;
    return ObiCreateHandle(Object, Process, Access, Handle);
}

NSTATUS KRNLAPI ObOpenHandleByName(
    IN PEPROCESS Process,
    IN OBTYPE ObjectType,
    IN char* ObjectName, // Object name inside the object type namespace
    IN UINT64 Access,
    IN HANDLE* Handle
) {
    
    // NOTE : All libc functions will be replaced with high performance equivalents
    UINT16 len = strlen(ObjectName);
    if(!len) return STATUS_INVALID_PARAMETER;
    POBJECT Obj = _ObObjectTypeStarts[ObjectType];
    

    while(Obj) {
        if(Obj->ObjectNameLength == len && memcmp(Obj->ObjectName, ObjectName, Obj->ObjectNameLength)) {
            break;
        }
        Obj = Obj->TypeContinuation;
    }
    if(!Obj) return STATUS_NOT_FOUND;

    return ObOpenHandle(Obj, Process, Access, Handle);    
}

NSTATUS KRNLAPI ObOpenHandleById(
    IN PEPROCESS Process,
    IN OBTYPE ObjectType,
    IN UINT64 ObjectId, // Object Id Inside the object type namespace
    IN UINT64 Access,
    IN HANDLE* Handle
) {
    POBJECT Obj = _ObObjectTypeStarts[ObjectType];
    while(Obj) {
        if(Obj->ObjectId == ObjectId) break;
        Obj = Obj->TypeContinuation;
    }
    if(!Obj) return STATUS_NOT_FOUND;

    return ObOpenHandle(Obj, Process, Access, Handle);
}

NSTATUS KRNLAPI ObOpenHandleByAddress(
    PEPROCESS Process,
    void* Address,
    IN OBTYPE ObjectType,
    IN UINT64 Access,
    IN HANDLE* Handle
) {
    POBJECT Obj = _ObObjectTypeStarts[ObjectType];
    while(Obj) {
        if(Obj->Address == Address) break;
        Obj = Obj->TypeContinuation;
    }
    if(!Obj) return STATUS_NOT_FOUND;

    return ObOpenHandle(Obj, Process, Access, Handle);
}

BOOLEAN KRNLAPI ObCloseHandle(PEPROCESS Process, HANDLE Handle) {
    if(!Process) Process = KernelProcess;
    if(!ObCheckHandle(Handle) || ObiReferenceByHandle(Handle)->Process != Process) {
        return FALSE;
    }
    POBJECT_REFERENCE Ref = _ObHandleArray + (UINT64)Handle;
    ObLockHandle(Handle);

    // Unlink the reference
    UINT64 rflags = ExAcquireSpinLock(&Ref->Object->SpinLock);
    if(Ref->Object->References == Ref) Ref->Object->References = Ref->Next;
    else if(Ref->Object->ReferencesLastNode == Ref) Ref->Object->ReferencesLastNode = Ref->Previous;

    if(Ref->Previous) Ref->Previous->Next = Ref->Next;

    // Check if the object is not permanent and can be destroyed
    
    POBJECT Obj = Ref->Object;
    Obj->NumReferences--;
    Ref->Object->EventHandler(Ref->Process, OBJECT_EVENT_CLOSE, Handle, Ref->Access);

    if(!Obj->NumReferences && !(Obj->Characteristics & OBJECT_PERMANENT)) {
        Ref->Object->EventHandler(NULL, OBJECT_EVENT_DESTROY, (HANDLE)Ref->Object, Ref->Access);
       
        ObiFreeObject(Obj);
        __writeeflags(rflags);

    } else {
        ExReleaseSpinLock(&Ref->Object->SpinLock, rflags);
    }

    // De-Allocate the reference
    ObjZeroMemory(Ref);
    
    _interlockedbittestandreset64(_ObHandleAllocationTable + ((UINT64)Handle >> 6), (UINT64)Handle & 0x3F);
    return TRUE;
}

BOOLEAN KRNLAPI ObCheckHandle(HANDLE _Handle) {
    UINT64 Handle = (UINT64)_Handle;
    if(Handle >= _ObMaxHandles) return FALSE;

    return TRUE;
}

PVOID KRNLAPI ObGetObjectByHandle(HANDLE _Handle, POBJECT* Object, OBTYPE ObjectType) {
    if(!ObCheckHandle(_Handle)) return NULL;
    *Object = _ObHandleArray[(UINT64)_Handle].Object;
    if((*Object)->ObjectType != ObjectType) return NULL;
    return (*Object)->Address;
}

BOOLEAN KRNLAPI ObLockHandle(HANDLE _Handle) {
    if(!ObCheckHandle(_Handle)) return FALSE;
    POBJECT_REFERENCE Ref = ObiReferenceByHandle(_Handle);

    /*
    * SECURITY_BUG_CHECK : Possible BUG
    */
    while(_interlockedbittestandset(&Ref->Mutex, 0)) {
        _mm_pause();
        if(!ObCheckHandle(_Handle)) return FALSE;
    }
    return TRUE;
}

BOOLEAN KRNLAPI ObUnlockHandle(HANDLE _Handle) {
    if(!ObCheckHandle(_Handle)) return FALSE;
    POBJECT_REFERENCE Ref = ObiReferenceByHandle(_Handle);
    _interlockedbittestandreset64(&Ref->Mutex, 0);
    return TRUE;
}