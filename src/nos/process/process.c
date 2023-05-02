#include <nos/process/internal.h>
volatile UINT64 LastThreadId = 0, LastProcessId = 1; // Kernel Process is PID 0



THREAD_LIST ThreadList = {INITIAL_MUTEX, 0, {0}, NULL, THREAD_LIST_MAGIC};
PROCESS_LIST ProcessList = {INITIAL_MUTEX, 1, {
    {
        // Kernel Process
        NULL, 0, 0, 0,
        L"System Kernel.", L"NewOS/System/noskx64.exe", L"System kernel process.",
        NULL, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, {0}, &ProcessList, 0
    }, 0
}, NULL, PROCESS_LIST_MAGIC};

SUBSYSTEM_DESCRIPTOR Subsystems[0x100] = {0};


NSTATUS KRNLAPI KeCreateProcess(
    IN OPT PROCESS* Parent,
    OUT OPT UINT64* ProcessId,
    IN UINT64 Flags,
    IN UINT8 Subsystem,
    IN UINT16* DisplayName,
    IN UINT16* Path,
    IN OPT UINT16* Description,
    IN void* EntryPoint
) {
    // Runtime Checks
    if(!Subsystems[Subsystem].Flags) return STATUS_SUBSYSTEM_NOT_PRESENT;
    if(Parent && !KeCheckProcess(Parent)) return STATUS_INVALID_PARAMETER;
    // Process allocation
    PROCESS* Process;
    PROCESS_LIST* Processes = &ProcessList;
    unsigned long Index;
    while(Processes) {
        KeMutexWait(NULL, &Processes->Mutex, 0);
        if(_BitScanForward64(&Index, ~Processes->Present)) {
            _bittestandset64(&Processes->Present, Index);
            Process = &Processes->Processes[Index];
            KeMutexRelease(NULL, &Processes->Mutex);
            break;
        }
        if(!Processes->Next) {
            SerialLog("Processes->Next");
            while(1);
        }
        KeMutexRelease(NULL, &Processes->Mutex);
        Processes = Processes->Next;
    }
    // Setting up the process
    Process->ProcessId = _InterlockedIncrement64(&LastProcessId) - 1;
    Process->Parent = Parent;
    Process->Flags = Flags;
    Process->ProcessDisplayName = DisplayName;
    Process->Path = Path;
    Process->ProcessDescription = Description;
    Process->Subsystem = Subsystem;

    Process->ParentList = Processes;
    Process->ParentListIndex = Index;

    if(ProcessId) *ProcessId = Process->ProcessId;
    // Create main thread
    NSTATUS s = KeCreateThread(Process, NULL, 0, EntryPoint);
    if(NERROR(s)) {
        // TODO : Remove process
        while(1);
    }
    return STATUS_SUCCESS;
}




BOOLEAN KeCheckProcess(PROCESS* Process) {
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
PROCESS* KiGetProcessById(UINT64 ProcessId) {
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