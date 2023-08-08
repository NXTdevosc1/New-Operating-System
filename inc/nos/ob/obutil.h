#include <nos/nos.h>
#include <nos/ob/ob.h>

UINT64 _ObMaxHandles;
UINT64 _ObNumHandles;


volatile UINT64* _ObAllocationTable;
volatile UINT64* _ObHandleAllocationTable;

POBJECT _ObObjectArray;
POBJECT_REFERENCE _ObHandleArray;

UINT64 _ObAllocationTableSize;
UINT64 _ObObjectArraySize;
UINT64 _ObHandleArraySize;

POBJECT _ObObjectTypeStarts[OB_MAX_NUMBER_OF_TYPES];
POBJECT _ObObjectTypeEnds[OB_MAX_NUMBER_OF_TYPES];

#define ObiHandleByReference(_Ref) ((HANDLE)(((UINT64)_Ref - (UINT64)_ObHandleArray) / sizeof(OBJECT_REFERENCE_DESCRIPTOR)))
#define ObiReferenceByHandle(_Handle) (_ObHandleArray + (UINT64)_Handle)
OBJECT_TYPE_DESCRIPTOR _ObObjectTypes[OB_MAX_NUMBER_OF_TYPES];

POBJECT ObiAllocateObject();
void ObiFreeObject(POBJECT Object);

NSTATUS ObiCreateHandle(POBJECT Object, PEPROCESS Process, UINT64 Access, HANDLE* _OutHandle);
void ObiDeleteHandle(HANDLE Handle);

void ObiLinkChildObject(POBJECT Parent, POBJECT Child);
void ObiUnlinkChildObject(POBJECT Parent, POBJECT Child);