#include <nos/process/internal.h>
volatile UINT64 LastThreadId = 0, LastProcessId = 1; // Kernel Process is PID 0



THREAD_LIST ThreadList = {INITIAL_MUTEX, 0, {0}, NULL, THREAD_LIST_MAGIC};
PROCESS_LIST ProcessList = {INITIAL_MUTEX, 1, {0}, NULL, PROCESS_LIST_MAGIC};

SUBSYSTEM_DESCRIPTOR Subsystems[0x100] = {0};


NSTATUS KRNLAPI ExCreateProcess(
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
    if(ParentProcess && !ExProcessExists(ParentProcess)) return STATUS_INVALID_PARAMETER;
    // Process allocation
    PEPROCESS Process;
    PROCESS_LIST* Processes = &ProcessList;
    unsigned long Index;
    while(Processes) {
        ExMutexWait(NULL, &Processes->Mutex, 0);
        if(_BitScanForward64(&Index, ~Processes->Present)) {
            _bittestandset64(&Processes->Present, Index);
            Process = &Processes->Processes[Index];
            ExMutexRelease(NULL, &Processes->Mutex);
            break;
        }
        if(!Processes->Next) {
            SerialLog("Processes->Next");
            while(1);
        }
        ExMutexRelease(NULL, &Processes->Mutex);
        Processes = Processes->Next;
    }
    // Setting up the process
    Process->ProcessId = _InterlockedIncrement64(&LastProcessId) - 1;
    Process->Parent = ParentProcess;
    Process->Flags = CreateFlags;
    Process->ProcessDisplayName = DisplayName;
    Process->Path = Path;
    Process->Subsystem = Subsystem;

    Process->ParentList = Processes;
    Process->ParentListIndex = Index;

    if(OutProcess) *OutProcess = Process;
    // Create main thread
    NSTATUS s = ExCreateThread(Process, NULL, 0, EntryPoint);
    if(NERROR(s)) {
        // TODO : Remove process
        while(1);
    }
    return STATUS_SUCCESS;
}




BOOLEAN ExProcessExists(PEPROCESS Process) {
    if(!Process) return FALSE;
    // TODO : Check if the page of the pointer is accessible
    // Check if the process is in the process list
    PROCESS_LIST* l = Process->ParentList;
    if(l->Magic != PROCESS_LIST_MAGIC ||
    Process->ParentListIndex > 63 ||
    Process != &l->Processes[Process->ParentListIndex]
    ) return FALSE;
    
    return TRUE;
}

// Finds the process and returns raw pointer
PEPROCESS ExGetProcessById(UINT64 ProcessId) {
    PROCESS_LIST* pl = &ProcessList;
    UINT64 m;
    unsigned long Index;
    while(pl) {
        m = pl->Present;
        while(_BitScanForward64(&Index, m)) {
            _bittestandreset64(&m, Index);
            if(pl->Processes[Index].ProcessId == ProcessId) return &pl->Processes[Index];
        }
        pl = pl->Next;
    }
    return NULL;
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
