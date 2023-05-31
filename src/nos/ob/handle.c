#include <nos/ob/obutil.h>

NSTATUS KRNLAPI ObOpenHandle(
    IN POBJECT Object,
    IN PEPROCESS Process,
    IN UINT64 Access,
    IN HANDLE* Handle
) {
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

void ObCloseHandle(HANDLE Handle) {
    POBJECT_REFERENCE Ref = _ObHandleArray + (UINT64)Handle;
    ObLockHandle(Handle);
    // Unlink the reference
    UINT64 rflags = ExAcquireSpinLock(&Ref->Object->SpinLock);
    if(Ref->Object->References == Ref) Ref->Object->References = Ref->Next;
    else if(Ref->Object->ReferencesLastNode == Ref) Ref->Object->ReferencesLastNode = Ref->Previous;

    Ref->Previous->Next = Ref->Next;

    // Check if the object is not permanent and can be destroyed
    
    POBJECT Obj = Ref->Object;
    Obj->NumReferences--;
    Ref->Object->OnClose(Handle, Ref->Process, Ref->Access);

    if(!Obj->NumReferences && !(Obj->Characteristics & OBJECT_PERMANENT)) {
        Obj->OnDestroy(Obj);
        ObiFreeObject(Obj);
        __writeeflags(rflags);
    } else {
        ExReleaseSpinLock(&Ref->Object->SpinLock, rflags);
    }

    // De-Allocate the reference
    ObjZeroMemory(Ref);
    _interlockedbittestandreset64(_ObHandleAllocationTable + ((UINT64)Handle >> 6), (UINT64)Handle & 0x3F);

}

BOOLEAN ObCheckHandle(HANDLE _Handle) {
    UINT64 Handle = (UINT64)_Handle;
    if(Handle >= _ObMaxHandles) return FALSE;

    return TRUE;
}

POBJECT ObGetObjectByHandle(HANDLE _Handle) {
    UINT64 Handle = (UINT64)_Handle;
    return _ObHandleArray[Handle].Object;
}

BOOLEAN ObLockHandle(HANDLE _Handle) {
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

BOOLEAN ObUnlockHandle(HANDLE _Handle) {
    if(!ObCheckHandle(_Handle)) return FALSE;
    POBJECT_REFERENCE Ref = ObiReferenceByHandle(_Handle);
    _interlockedbittestandreset64(&Ref->Mutex, 0);
    return TRUE;
}