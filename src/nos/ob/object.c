#include <nos/ob/obutil.h>

NSTATUS KRNLAPI ObCreateObject(
    // All fields are required
    IN OPT POBJECT ParentObject,
    OUT POBJECT *_OutObject,
    IN UINT64 Characteristics,
    IN OBTYPE ObjectType,
    IN OPT char *ObjectName,
    IN UINT64 Size,
    IN OBJECT_EVT_HANDLER EventHandler // Typically handles OnOpen/OnClose Events
)
{
    if (!_ObObjectTypes[ObjectType].TypeName || !EventHandler)
        return STATUS_INVALID_PARAMETER;
    POBJECT Object = ObiAllocateObject();
    if (!Object)
        return STATUS_NO_FREE_SLOTS;

    Object->Address = KlAllocatePool(Size, 0);
    if (!Object->Address)
    {
        ObiFreeObject(Object);
        return STATUS_OUT_OF_MEMORY;
    }
    ZeroMemory(Object->Address, Size);

    // Set Object Data
    Object->Characteristics = Characteristics | OBJECT_PRESENT;
    Object->ObjectType = ObjectType;
    if (Object->ObjectName)
    {
        Object->ObjectName = ObjectName;
        Object->ObjectNameLength = strlen(ObjectName);
    }
    Object->EventHandler = EventHandler;
    Object->ObjectId = _InterlockedIncrement64(&_ObObjectTypes[ObjectType].TotalCreatedObjects) - 1;

    Object->Parent = ParentObject;

    // Link Object Type
    if (!_ObObjectTypeStarts[ObjectType])
    {
        _ObObjectTypeStarts[ObjectType] = Object;
        _ObObjectTypeEnds[ObjectType] = Object;
    }
    else
    {
        _ObObjectTypeEnds[ObjectType]->TypeContinuation = Object;
        _ObObjectTypeEnds[ObjectType] = Object;
    }
    _interlockedincrement64(&_ObObjectTypes[ObjectType].NumObjects);

    if (ParentObject)
    {
        ObiLinkChildObject(ParentObject, Object);
    }
    *_OutObject = Object;
    return STATUS_SUCCESS;
}

BOOLEAN KRNLAPI ObDestroyObject(
    IN POBJECT Object,
    IN BOOLEAN Force)
{
    UINT64 rflags = ExAcquireSpinLock(&Object->SpinLock);
    // Object is in use
    if (!Force && Object->NumReferences)
    {
        ExReleaseSpinLock(&Object->SpinLock, rflags);
        return FALSE;
    }

    if (Force && Object->NumReferences)
    {
        // Remove the references
        POBJECT_REFERENCE Ref = Object->References;
        while (Ref)
        {
            POBJECT_REFERENCE NextRef = Ref->Next;
            ObCloseHandle(Ref->Process, ObiHandleByReference(Ref));
            Ref = NextRef; // because CloseHandle deletes the reference
        }
    }

    if (!(Object->Characteristics & OBJECT_PERMANENT))
    {
        Object->EventHandler(NULL, OBJECT_EVENT_DESTROY, (HANDLE)Object, 0);
    }

    // Destroy all sub objects
    POBJECT Child = Object->FirstChild;
    while (Child)
    {
        ObDestroyObject(Child, TRUE);
        Child = Child->NextChild;
    }
    KlFreePool(Object->Address);
    ObiFreeObject(Object);
    __writeeflags(rflags);

    return TRUE;
}

UINT64 KRNLAPI ObEnumerateObjects(
    IN POBJECT Object,
    IN OPT OBTYPE ObjectType, // Set to UNDEFINED_OBJECT_TYPE to enumerate all objects
    OUT POBJECT *_OutObject,
    OUT OPT POBJECT *_FirstChild, // Allowing Sub-Enumeration through objects
    UINT64 _EnumVal)
{

    if (_EnumVal == (UINT64)-1)
        return 0;
    if (Object)
    {
        if (_EnumVal)
        {
            *_OutObject = (POBJECT)_EnumVal;
            _EnumVal = (UINT64)((*_OutObject)->NextChild);
        }
        else
        {
            *_OutObject = (POBJECT)Object->FirstChild;
            _EnumVal = (UINT64)Object->FirstChild->NextChild;
        }
        if (!_EnumVal)
            _EnumVal = (UINT64)-1; // This is the last object

        if ((*_OutObject)->ObjectType != ObjectType)
        {
            // walk the objects until we find an object with the same type
            return ObEnumerateObjects(Object, ObjectType, _OutObject, _FirstChild, _EnumVal);
        }
    }
    else
    {
        if (ObjectType == UNDEFINED_OBJECT_TYPE)
        {
            // This uses an array
            while (!((*_OutObject) = ObGetRawObject(_EnumVal)) && _EnumVal < _ObMaxHandles)
            {
                _EnumVal++;
            }
            if (_EnumVal == _ObMaxHandles)
                _EnumVal = 0;
        }
        else
        {
            // This uses a continuous chain
            *_OutObject = ObGetRawObjectByType(ObjectType, (UINT64)_EnumVal);
            ((UINT64)_EnumVal)++;
            if (!(*_OutObject))
                _EnumVal = 0;
        }
    }
    return _EnumVal;
}

POBJECT KRNLAPI ObGetRawObject(IN UINT64 Index)
{
    if (Index >= _ObMaxHandles)
        return NULL;
    return _ObObjectArray + Index;
}

POBJECT KRNLAPI ObGetRawObjectByType(IN OBTYPE Type, IN UINT64 Index)
{
    if (!_ObObjectTypes[Type].TypeName)
        return NULL;
    POBJECT Obj = _ObObjectTypeStarts[Type];
    while (Index && Obj)
    {
        Index--;
        Obj = Obj->TypeContinuation;
    }
    return Obj;
}

PVOID KRNLAPI ObGetAddress(POBJECT Object)
{
    return Object->Address;
}

// Cannot remove type after being registered
BOOLEAN KRNLAPI ObRegisterObjectType(
    IN OBTYPE Type,
    IN char *TypeName,
    IN UINT16 MajorVersion,
    IN UINT16 MinorVersion)
{
    if (_ObObjectTypes[Type].TypeName)
        return FALSE;
    if (strlen(TypeName) > MAX_OBJECT_TYPE_NAME_LENGTH)
        return FALSE;
    _ObObjectTypes[Type].TypeName = TypeName;
    _ObObjectTypes[Type].MajorVersion = MajorVersion;
    _ObObjectTypes[Type].MinorVersion = MinorVersion;

    return TRUE;
}

BOOLEAN KRNLAPI ObReadObjectTypeInformation(
    IN OBTYPE Type,
    IN char *TypeName,
    IN UINT16 *MajorVersion,
    IN UINT16 *MinorVersion)
{
    OBJECT_TYPE_DESCRIPTOR *obt = &_ObObjectTypes[Type];
    if (!obt->TypeName)
        return FALSE;
    strcpy(TypeName, obt->TypeName);
    *MajorVersion = obt->MajorVersion;
    *MinorVersion = obt->MinorVersion;

    return TRUE;
}

BOOLEAN KRNLAPI ObCheckObject(POBJECT Object)
{
    if ((UINT64)Object < (UINT64)_ObObjectArray ||
        (UINT64)Object > ((UINT64)_ObObjectArray + _ObObjectArraySize - sizeof(OBJECT_DESCRIPTOR)))
        return FALSE;

    if (Object->Characteristics & OBJECT_PRESENT)
        return TRUE;

    return FALSE;
}