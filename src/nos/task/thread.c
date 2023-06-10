#include <nos/nos.h>
#include <nos/task/process.h>

NSTATUS ThreadEvtHandler(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access) {
    return STATUS_SUCCESS;
}

NSTATUS KRNLAPI KeCreateThread(
    IN PEPROCESS Process,
    OUT PETHREAD* OutThread,
    IN UINT64 Flags,
    IN void* EntryPoint
) {
    if(!KeProcessExists(Process)) return STATUS_INVALID_PARAMETER;
    // Allocate the thread
    PETHREAD Thread;
    POBJECT Ob;
    NSTATUS s = ObCreateObject(
        Process->ProcessObject,
        &Ob,
        0,
        OBJECT_THREAD,
        NULL,
        sizeof(ETHREAD),
        ThreadEvtHandler
    );

    if(NERROR(s)) {
        KDebugPrint("failed to create object thread");
        while(1) __halt();
    }


    Thread = Ob->Address;
    // Setting up the thread
    Thread->ThreadObject = Ob;
    Thread->Process = Process;
    Thread->ThreadId = Ob->ObjectId;
    Thread->Flags = Flags;
    Thread->Ready.Thread = Thread;
    _InterlockedIncrement64(&Process->NumberOfThreads);

    if(OutThread) *OutThread = Thread;
    
    // Setup subsystem entry point
    Thread->Registers.rip = (UINT64)Subsystems[Process->Subsystem].EntryPoint;
    Thread->Registers.rcx = (UINT64)EntryPoint; // Parameter0

    return STATUS_SUCCESS;
}

BOOLEAN KRNLAPI KeThreadExists(PETHREAD Thread) {
    UINT64 f = PAGE_WRITE_ACCESS;
    if(!Thread || !KeCheckMemoryAccess(KernelProcess, Thread, sizeof(ETHREAD), &f)) return FALSE;
    if(!ObCheckObject(Thread->ThreadObject) || Thread->ThreadObject->ObjectType != OBJECT_THREAD ||
    Thread->ThreadObject->Address != Thread
    ) return FALSE;

    return TRUE;
}

#include <nos/ob/obutil.h>

// Finds the thread and returns raw pointer
PETHREAD KRNLAPI KeGetThreadById(UINT64 ThreadId) {
    HANDLE h;
    if(NERROR(ObOpenHandleById(KernelProcess, OBJECT_THREAD, ThreadId, 0, &h))) return NULL;
    PETHREAD t = ObiReferenceByHandle(h)->Object->Address;
    ObCloseHandle(KernelProcess, h);
    return t;
}

PETHREAD KRNLAPI KeWalkThreads(PEPROCESS Process, UINT64* EnumVal) {
    POBJECT obj;
    *EnumVal = ObEnumerateObjects(Process->ProcessObject, OBJECT_THREAD, &obj, NULL, *EnumVal);
    if(!(*EnumVal)) return NULL;

    return obj->Address;
}

BOOLEAN KRNLAPI KeSetStaticPriority(PETHREAD Thread, UINT StaticPriority) {
    Thread->StaticPriority = StaticPriority;
    if(Thread->DynamicPriority < StaticPriority) Thread->DynamicPriority = StaticPriority;
    if(!Thread->Ready.Ready) {
        UINT64 rf = __readeflags();
        _disable();
        ScBottomAddReadyThread(Thread);
        Thread->Ready.Ready = TRUE;
        __writeeflags(rf);
    }
    return TRUE;
}