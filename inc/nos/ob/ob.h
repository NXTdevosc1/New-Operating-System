// NOS Object Manager
#pragma once
#include <nosdef.h>
#include <nos/task/process.h>

#include <ob.h>


// HANDLE is a data type that stores the index to the reference descriptor
typedef struct _OBJECT_REFERENCE_DESCRIPTOR {
    BOOLEAN Mutex; // Used when a function uses the handle
    POBJECT Object;
    PEPROCESS Process;
    /*128 Bit User custumizable Access field*/
    UINT64 Access;
    POBJECT_REFERENCE Previous;
    POBJECT_REFERENCE Next;
} OBJECT_REFERENCE_DESCRIPTOR, *POBJECT_REFERENCE;


typedef struct _OBJECT_DESCRIPTOR {
    UINT64 Characteristics;
    UINT64 ObjectId;
    SPINLOCK SpinLock;
    char* ObjectName;
    UINT16 ObjectNameLength;

    OBTYPE ObjectType;
    UINT NumReferences;
    void* Address;

    POBJECT_REFERENCE References;    
    POBJECT_REFERENCE ReferencesLastNode;

    OBJECT_EVT_HANDLER EventHandler;

    POBJECT FirstChild;
    POBJECT LastChild;

    POBJECT Parent;
    POBJECT NextChild;
    POBJECT PreviousChild;

    // Next object from the same type
    POBJECT TypeContinuation;
} OBJECT_DESCRIPTOR, *POBJECT;


// Used to provide information for the user about each object type
typedef struct _OBJECT_TYPE_DESCRIPTOR {
    UINT16 MajorVersion;
    UINT16 MinorVersion;
    char* TypeName;
    volatile UINT64 NumObjects;
    volatile UINT64 TotalCreatedObjects;
} OBJECT_TYPE_DESCRIPTOR, *POBJECT_TYPE;

void ObInitialize();
