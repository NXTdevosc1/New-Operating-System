#include <nos/ob/obutil.h>
UINT64 _ObMaxHandles = 0;
UINT64 _ObNumHandles = 0;


volatile UINT64* _ObAllocationTable = NULL;
volatile UINT64* _ObHandleAllocationTable = NULL;

POBJECT _ObObjectArray = NULL;
POBJECT_REFERENCE _ObHandleArray = NULL;

UINT64 _ObAllocationTableSize = 0, _ObObjectArraySize = 0;
UINT64 _ObHandleArraySize = 0;

POBJECT _ObObjectTypeStarts[OB_MAX_NUMBER_OF_TYPES] = {0};
POBJECT _ObObjectTypeEnds[OB_MAX_NUMBER_OF_TYPES] = {0};

OBJECT_TYPE_DESCRIPTOR _ObObjectTypes[OB_MAX_NUMBER_OF_TYPES] = {0};

void ObInitialize() {
    KDebugPrint("Object Manager : Init");
    _ObMaxHandles = NosInitData->BootHeader->MaxHandles;
    if(NosInitData->BootHeader->MaxHandles & 0x3F) {
        KDebugPrint("Object Manager ERROR : MaxHandles should be aligned by 64 entries.");
        while(1) __halt();
    }

    _ObAllocationTableSize = AlignForward(_ObMaxHandles / 8, 0x1000);

    _ObObjectArraySize = AlignForward(_ObMaxHandles * sizeof(OBJECT_DESCRIPTOR), 0x1000);
    
    _ObHandleArraySize = AlignForward(_ObMaxHandles * sizeof(OBJECT_REFERENCE_DESCRIPTOR), 0x1000);

    _ObAllocationTable = MmAllocateMemory(KernelProcess, _ObAllocationTableSize >> 12, PAGE_WRITE_ACCESS, PAGE_CACHE_DISABLE);
    _ObHandleAllocationTable = MmAllocateMemory(KernelProcess, _ObAllocationTableSize >> 12, PAGE_WRITE_ACCESS, PAGE_CACHE_DISABLE);

    _ObObjectArray = MmAllocateMemory(KernelProcess, _ObObjectArraySize >> 12, PAGE_WRITE_ACCESS, PAGE_CACHE_DISABLE);
    _ObHandleArray = MmAllocateMemory(KernelProcess, _ObHandleArraySize >> 12, PAGE_WRITE_ACCESS, PAGE_CACHE_DISABLE);

    if(!_ObAllocationTable || !_ObObjectArray || !_ObHandleAllocationTable || !_ObHandleArray) {
        KDebugPrint("Object Manager : Failed to allocate resources.");
        while(1) __halt();
    }
    ZeroMemory(_ObAllocationTable, _ObAllocationTableSize);
    ZeroMemory(_ObHandleAllocationTable, _ObAllocationTableSize);
    ZeroMemory(_ObObjectArray, _ObHandleArraySize);
    ZeroMemory(_ObHandleArray, _ObHandleArraySize);

    KDebugPrint("Object Manager : Allocate Table : %u Bytes, Object Array : %u Bytes , Handle Array : %u Bytes", _ObAllocationTableSize, _ObObjectArraySize, _ObHandleArraySize);


}

NSTATUS ObiCreateHandle(POBJECT Object, PEPROCESS Process, UINT64 Access, HANDLE* _OutHandle) {
    // Allocate Handle
    POBJECT_REFERENCE Reference = NULL;
    {
        const UINT64 _max = _ObAllocationTableSize >> 3;
        volatile UINT64* Bitmask = _ObHandleAllocationTable;
        UINT Index;
        for(UINT64 i = 0;i<_max;i++, Bitmask++) {
    retry:
            if(_BitScanForward64(&Index, ~(*Bitmask))) {
                if(_interlockedbittestandset64(Bitmask, Index)) goto retry;
                Reference = _ObHandleArray + (i << 6) + Index;
                break;
            }
        }
        if(!Reference) return STATUS_NO_FREE_SLOTS;
    }
    // Set reference data
    Reference->Object = Object;
    Reference->Process = Process;
    Reference->Access = Access;
    HANDLE Handle = (HANDLE)(((UINT64)Reference - (UINT64)_ObHandleArray) / sizeof(OBJECT_REFERENCE_DESCRIPTOR));
    NSTATUS s;
    if(NERROR((s = Object->OnOpen(Handle, Process, Access)))) {
        // De-Allocate the reference
        ObjZeroMemory(Reference);
        _interlockedbittestandreset64(_ObHandleAllocationTable + ((UINT64)Handle >> 6), (UINT64)Handle & 0x3F);
        return s;
    }
    // Link the handle to the object
    UINT64 rflags = ExAcquireSpinLock(&Object->SpinLock);
    if(Object->References) {
        Object->ReferencesLastNode->Next = Reference;
        Reference->Previous = Object->ReferencesLastNode;
        Object->ReferencesLastNode = Reference;
    } else {
        Object->References = Reference;
        Object->ReferencesLastNode = Reference;
    }
    Object->NumReferences++;
    ExReleaseSpinLock(&Object->SpinLock, rflags);
    *_OutHandle = Handle;
    return STATUS_SUCCESS;
}



POBJECT ObiAllocateObject() {
    const UINT64 _max = _ObAllocationTableSize >> 3;
    volatile UINT64* Bitmask = _ObAllocationTable;
    UINT Index;
    for(UINT64 i = 0;i<_max;i++, Bitmask++) {
retry:
        if(_BitScanForward64(&Index, ~(*Bitmask))) {
            if(_interlockedbittestandset64(Bitmask, Index)) goto retry;
            POBJECT Object = _ObObjectArray + (i << 6) + Index;
            Object->Characteristics = OBJECT_PRESENT;
            return Object;
        }
    }
    return NULL;
}

void ObiFreeObject(POBJECT Object) {
    UINT64 Index = (((UINT64)Object - (UINT64)_ObObjectArray) / sizeof(OBJECT_DESCRIPTOR));
    _interlockedbittestandreset64(_ObAllocationTable + ((UINT64)Index >> 6), (UINT64)Index & 0x3F);
}


