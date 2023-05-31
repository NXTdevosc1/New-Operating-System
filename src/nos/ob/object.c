#include <nos/ob/obutil.h>

NSTATUS KRNLAPI ObCreateObject(
// All fields are required
    IN UINT64 Characteristics,
    IN OBTYPE ObjectType,
    IN char* ObjectName,
    IN void* Address,
    IN OBJECT_ONOPEN OnOpen,
    IN OBJECT_ONCLOSE OnClose,
    IN OBJECT_ONDESTROY OnDestroy // not required if characteristics & OB_PERMANENT
) {
    if(!_ObObjectTypes[ObjectType].TypeName) return STATUS_INVALID_PARAMETER;
    POBJECT Object = ObiAllocateObject();
    if(!Object) return STATUS_NO_FREE_SLOTS;

    // Set Object Data
    Object->Characteristics = Characteristics;
    Object->ObjectType = ObjectType;
    Object->ObjectName = ObjectName;
    Object->ObjectNameLength = strlen(ObjectName);
    Object->Address = Address;
    Object->OnOpen = OnOpen;
    Object->OnClose = OnClose;
    Object->OnDestroy = OnDestroy;

    // Link Object Type
    if(!_ObObjectTypeStarts[ObjectType]) {
        _ObObjectTypeStarts[ObjectType] = Object;
        _ObObjectTypeEnds[ObjectType] = Object;
    } else {
        _ObObjectTypeEnds[ObjectType]->TypeContinuation = Object;
        _ObObjectTypeEnds[ObjectType] = Object;
    }
    _interlockedincrement64(&_ObObjectTypes[ObjectType].NumObjects);
    return STATUS_SUCCESS;
}

BOOLEAN KRNLAPI ObDestroyObject(
    IN POBJECT Object,
    IN BOOLEAN Force
) {
    UINT64 rflags = ExAcquireSpinLock(&Object->SpinLock);
    // Object is in use
    if(!Force && Object->NumReferences) return FALSE;
    
    if(Force && Object->NumReferences) {
        // Remove the references
        POBJECT_REFERENCE Ref = Object->References;
        while(Ref) {
            POBJECT_REFERENCE NextRef = Ref->Next;
            ObCloseHandle(ObiHandleByReference(Ref));
            Ref = NextRef; // because CloseHandle deletes the reference
        }
    }

    if(!(Object->Characteristics & OBJECT_PERMANENT)) {
        Object->OnDestroy(Object);
    }
    ObiFreeObject(Object);
    __writeeflags(rflags);

    return TRUE;
}

POBJECT KRNLAPI ObGetRawObject(IN UINT64 Index) {
    if(Index >= _ObMaxHandles) return NULL;
    return _ObObjectArray + Index;
}

POBJECT KRNLAPI ObGetRawObjectByType(IN OBTYPE Type, IN UINT64 Index) {
    if(!_ObObjectTypes[Type].TypeName) return NULL;
    POBJECT Obj = _ObObjectTypeStarts[Type];
    while(Index && Obj) {
        Index--;
        Obj = Obj->TypeContinuation;
    }
    return Obj;
}

// Cannot remove type after being registered
BOOLEAN KRNLAPI ObRegisterObjectType(
    IN OBTYPE Type,
    IN char* TypeName,
    IN UINT16 MajorVersion,
    IN UINT16 MinorVersion) {
        if(_ObObjectTypes[Type].TypeName) return FALSE;
    if(strlen(TypeName) > MAX_OBJECT_TYPE_NAME_LENGTH) return FALSE;
    _ObObjectTypes[Type].TypeName = TypeName;
    _ObObjectTypes[Type].MajorVersion = MajorVersion;
    _ObObjectTypes[Type].MinorVersion = MinorVersion;

    return TRUE;
}

BOOLEAN KRNLAPI ObReadObjectTypeInformation(
    IN OBTYPE Type,
    IN char* TypeName,
    IN UINT16* MajorVersion,
    IN UINT16* MinorVersion
) {
    OBJECT_TYPE_DESCRIPTOR* obt = &_ObObjectTypes[Type];
    if(!obt->TypeName) return FALSE;
    strcpy(TypeName, obt->TypeName);
    *MajorVersion = obt->MajorVersion;
    *MinorVersion = obt->MinorVersion;

    return TRUE;
}