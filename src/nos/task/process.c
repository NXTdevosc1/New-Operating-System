#include <nos/nos.h>
#include <nos/task/process.h>

SUBSYSTEM_DESCRIPTOR Subsystems[0x100] = {0};

NSTATUS ProcessEvt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access) {
    return STATUS_SUCCESS;
}

NSTATUS KRNLAPI KeCreateProcess(
    IN OPT PEPROCESS ParentProcess,
    OUT PEPROCESS* OutProcess,
    IN UINT64 CreateFlags,
    IN UINT8 Subsystem,
    IN UINT16* DisplayName,
    IN UINT16* Path,
    IN void* EntryPoint
) {
    // Runtime Checks
    if(!Subsystems[Subsystem].Flags) return STATUS_SUBSYSTEM_NOT_PRESENT;
    POBJECT ObParent = NULL;
    POBJECT Ob;
    if(ParentProcess) {
        if(!KeProcessExists(ParentProcess)) return STATUS_INVALID_PARAMETER;
        ObParent = ParentProcess->ProcessObject;
        _InterlockedIncrement64(&ParentProcess->NumberOfChildProcesses);
    }
    // Process allocation
    NSTATUS s = ObCreateObject(
        ObParent,
        &Ob,
        0,
        OBJECT_PROCESS,
        NULL,
        sizeof(EPROCESS),
        ProcessEvt
    );


    if(NERROR(s)) {
        KDebugPrint("failed to create object process status : %d", s);
        while(1) __halt();
    }


    PEPROCESS Process = Ob->Address;

    // Setting up the process
    Process->ProcessObject = Ob;
    Process->ProcessId = Ob->ObjectId;
    Process->Parent = ParentProcess;
    Process->Flags = CreateFlags;
    Process->ProcessDisplayName = DisplayName;
    Process->Path = Path;
    Process->Subsystem = Subsystem;


    if(OutProcess) *OutProcess = Process;

    if(Process->Subsystem == SUBSYSTEM_NATIVE) {
        // Kernel mode process
        Process->VmSearchStart = NosInitData->NosKernelImageBase;
        Process->VmSearchEnd = (void*)-1;
        Process->PageTable = GetCurrentPageTable();
    }
    // Create main thread
    s = KeCreateThread(Process, NULL, 0, EntryPoint);
    if(NERROR(s)) {
        // TODO : Remove process
        KDebugPrint("createprocess : failed to create thread status : %d", s);
        while(1);
    }

    return STATUS_SUCCESS;
}

#include <nos/ob/obutil.h>


BOOLEAN KeProcessExists(PEPROCESS Process) {
    UINT64 f = PAGE_WRITE_ACCESS;

    if(!Process || !KeCheckMemoryAccess(KernelProcess, Process, sizeof(EPROCESS), &f)) return FALSE;
    if(!ObCheckObject(Process->ProcessObject) || Process->ProcessObject->ObjectType != OBJECT_PROCESS ||
    Process->ProcessObject->Address != Process
    ) return FALSE;

    return TRUE;
}

// Finds the process and returns raw pointer
PEPROCESS KeGetProcessById(UINT64 ProcessId) {
    HANDLE h;
    if(NERROR(ObOpenHandleById(KernelProcess, OBJECT_PROCESS, ProcessId, 0, &h))) return NULL;
    PEPROCESS p = ObiReferenceByHandle(h)->Object->Address;
    ObCloseHandle(KernelProcess, h);
    return p;
}

NSTATUS KRNLAPI KeAcquireControlFlag(IN PEPROCESS Process, IN UINT64 ControlBit) {
    if(ControlBit > 63) return STATUS_INVALID_PARAMETER;
    if(!Process) Process = KernelProcess;
    while(_interlockedbittestandset64(&Process->ControlBitmask, ControlBit)) _mm_pause();
    return STATUS_SUCCESS;
}
NSTATUS KRNLAPI KeReleaseControlFlag(IN PEPROCESS Process, IN UINT64 ControlBit) {
    if(ControlBit > 63) return STATUS_INVALID_PARAMETER;
    if(!Process) Process = KernelProcess;
    _bittestandreset64(&Process->ControlBitmask, ControlBit);
    return STATUS_SUCCESS;
}

void KiInitMultitaskingSubsystem() {
    if(NERROR(KeCreateProcess(NULL,
    &KernelProcess,
    0,
    SUBSYSTEM_NATIVE,
    L"NOS System.",
    L"//NewOS/System/noskx64.exe",
    NULL
    ))) RaiseInitError(0);

    
}

BOOLEAN KRNLAPI KeSetProcessPriority(PEPROCESS Process, UINT Priority) {
    if(Priority > 5 && Priority != PROCESS_PRIORITY_CONSTANT_EXECUTION) return FALSE;
    PETHREAD Thread;
    UINT64 ev;
    Process->Priority = Priority;
    while((Thread = KeWalkThreads(Process, &ev))) {
        if(Thread->Ready.Ready) {
            KeSetStaticPriority(Thread, (Thread->StaticPriority % 5) * Priority);
        }
    }
    return TRUE;
}