#pragma once

#include <nosdef.h>


#define OB_MAX_NUMBER_OF_TYPES 0x1000
#define OB_MAX_TYPE 0xFFF

typedef UINT16 OBTYPE;


/*
 * Standard Object Types
*/

typedef enum _OBTYPES {
    OBJECT_HANDLE, // Mostly used on subobjects
    OBJECT_PROCESSOR,
    OBJECT_PROCESS,
    OBJECT_THREAD,
    OBJECT_FILE,
    OBJECT_DEVICE,
    OBJECT_TIMER,
    OBJECT_DRIVER,
    OBJECT_RCPORT,
    OBJECT_RESOURCE,
    OBJECT_EVENT,
    OBJECT_DRIVE,
    OBJECT_PARTITION,
    SYSTEM_DEFINED_OBJECT_MAX,
    UNDEFINED_OBJECT_TYPE = OB_MAX_NUMBER_OF_TYPES /*Max Type + 1*/
} OBTYPES;





typedef struct _OBJECT_DESCRIPTOR OBJECT_DESCRIPTOR, *POBJECT;
typedef struct _OBJECT_REFERENCE_DESCRIPTOR OBJECT_REFERENCE_DESCRIPTOR, *POBJECT_REFERENCE;

// Usually checks if the desired access field is valid or not
#define OBJECT_EVENT_OPEN 0
#define OBJECT_EVENT_CLOSE 1
#define OBJECT_EVENT_DESTROY 2 // All other values are invalid and Handle Contains the Pointer to the object

typedef NSTATUS (__cdecl *OBJECT_EVT_HANDLER)(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 DesiredAccess);
/*
 * Object Characteristics
*/


// Kernel does not delete the object after a reference count of 0


#define OBJECT_PRESENT 1
#define OBJECT_PERMANENT 2
#define OBJECT_ABSTRACT 4

#define HANDLE_ALL_ACCESS ((UINT64)-1)

// Handle API

NSTATUS KRNLAPI ObOpenHandleByName(
    IN PEPROCESS Process,
    IN OBTYPE ObjectType,
    IN char* ObjectName, // Object name inside the object type namespace
    IN UINT64 Access,
    IN HANDLE* Handle
);

NSTATUS KRNLAPI ObOpenHandleById(
    IN PEPROCESS Process,
    IN OBTYPE ObjectType,
    IN UINT64 ObjectId, // Object Id Inside the object type namespace
    IN UINT64 Access,
    IN HANDLE* Handle
);

NSTATUS KRNLAPI ObOpenHandleByAddress(
    PEPROCESS Process,
    void* Address,
    IN OBTYPE ObjectType,
    IN UINT64 Access,
    IN HANDLE* Handle
);


NSTATUS KRNLAPI ObOpenHandle(
    IN POBJECT Object,
    IN PEPROCESS Process,
    IN UINT64 Access,
    IN HANDLE* Handle
);

BOOLEAN KRNLAPI ObCloseHandle(PEPROCESS Process, HANDLE Handle);

BOOLEAN KRNLAPI ObCheckHandle(IN HANDLE _Handle);

BOOLEAN KRNLAPI ObLockHandle(IN HANDLE _Handle);
BOOLEAN KRNLAPI ObUnlockHandle(IN HANDLE _Handle);

// Object Management API

NSTATUS KRNLAPI ObCreateObject(
// All fields are required
    IN OPT POBJECT ParentObject,
    OUT POBJECT* _OutObject,
    IN UINT64 Characteristics,
    IN OBTYPE ObjectType,
    IN OPT char* ObjectName,
    IN UINT64 Size,
    IN OBJECT_EVT_HANDLER EventHandler // Typically handles OnOpen/OnClose Events
);

BOOLEAN KRNLAPI ObCheckObject(POBJECT Object);

/*
Return value : (UINT64)-1 on the last object, NULL when called after the last object
this makes it easier to be user in a loop like : while(ObEnumObjects(...))
*/
UINT64 KRNLAPI ObEnumerateObjects(
    IN POBJECT Object,
    IN OPT OBTYPE ObjectType, // Set to UNDEFINED_OBJECT_TYPE to enumerate all objects
    OUT POBJECT* _OutObject,
    OUT OPT POBJECT* _FirstChild, // Allowing Sub-Enumeration through objects
    UINT64 _EnumVal
);

BOOLEAN KRNLAPI ObDestroyObject(
    IN POBJECT Object,
    IN BOOLEAN Force
);

#define MAX_OBJECT_TYPE_NAME_LENGTH 0xFF

BOOLEAN KRNLAPI ObRegisterObjectType(
    IN OBTYPE Type,
    IN char* TypeName,
    IN UINT16 MajorVersion,
    IN UINT16 MinorVersion);

BOOLEAN KRNLAPI ObReadObjectTypeInformation(
    IN OBTYPE Type,
    IN char* TypeName,
    IN UINT16* MajorVersion,
    IN UINT16* MinorVersion
);

POBJECT KRNLAPI ObGetRawObject(IN UINT64 Index);
POBJECT KRNLAPI ObGetRawObjectByType(IN OBTYPE Type, IN UINT64 Index);
PVOID KRNLAPI ObGetObjectByHandle(HANDLE _Handle, POBJECT* Object, OBTYPE ObjectType);

PVOID KRNLAPI ObGetAddress(POBJECT Object);

#define RESOURCE_UNKNOWN 0
#define RESOURCE_EXECUTABLE_IMAGE 1

typedef struct _RESOURCE {
    POBJECT ObjectDescriptor;
    UINT ResourceType;
    UINT64 BufferSize;
    char ResourceBuffer[];
} RESOURCE, *PRESOURCE;