#include <nos/ob/obutil.h>


HANDLE ObOpenObjectByName(
    PEPROCESS Process,
    OBTYPE ObjectType,
    char* ObjectName,
    UINT64 Access
) {
    
    // NOTE : All libc functions will be replaced with high performance equivalents
    UINT16 len = strlen(ObjectName);

    POBJECT Obj = _ObObjectTypeStarts[ObjectType];
    

    while(Obj) {
        if(Obj->ObjectNameLength == len && memcmp(Obj->ObjectName, ObjectName, Obj->ObjectNameLength)) {
            break;
        }
        Obj = Obj->TypeContinuation;
    }
    if(!Obj) {
        KDebugPrint("ObAcquireObjectByName : Object Not Found, Creating Object...");
        while(1);
    }

    // Object Found
    HANDLE Handle;
    NSTATUS Status = ObiCreateHandle(Obj, Process, Access, &Handle);
    if(NERROR(Status)) return INVALID_HANDLE;
    
    return Handle;
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