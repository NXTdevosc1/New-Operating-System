#include <nos/process/internal.h>

NSTATUS KRNLAPI KeCreateThread(
    IN PROCESS* Process,
    OUT OPT UINT64* ThreadId,
    IN UINT64 Flags,
    IN void* EntryPoint
) {
    if(!KeCheckProcess(Process)) return STATUS_INVALID_PARAMETER;
    // Allocate the thread
    THREAD_LIST* Threads = &ThreadList;
    unsigned long Index;
    THREAD* Thread;
    while(Threads) {
        KeMutexWait(NULL, &Threads->Mutex, 0);
        if(_BitScanForward64(&Index, ~Threads->Present)) {
            _bittestandset64(&Threads->Present, Index);
            Thread = &Threads->Threads[Index];
            KeMutexRelease(NULL, &Threads->Mutex);
        }
        if(!Threads->Next) {
            SerialLog("Threads->next");
            while(1);
        }
        KeMutexRelease(NULL, &Threads->Mutex);
        Threads = Threads->Next;
    }
    // Setting up the thread
    Thread->Process = Process;
    Thread->ThreadId = _InterlockedIncrement64(&LastThreadId) - 1;
    Thread->Flags = Flags;
    Thread->ParentList = Threads;
    Thread->ParentListIndex = Index;

    if(ThreadId) *ThreadId = Thread->ThreadId;
    
    // Link the thread to the process
    while(_interlockedbittestandset64(&Process->ControlBitmask, PROCESS_CONTROL_LINK_THREAD)) _mm_pause();
    if(Process->LastThread) {
        Process->LastThread->NextThread = Thread;
        Process->LastThread = Thread;
    } else {
        // This is the main thread
        Process->Threads = Thread;
        Process->LastThread = Thread;
    }
    _bittestandreset64(&Process->ControlBitmask, PROCESS_CONTROL_LINK_THREAD);
    
    // Setup subsystem entry point
    Thread->Registers.rip = (UINT64)Subsystems[Process->Subsystem].EntryPoint;
    Thread->Registers.rcx = (UINT64)EntryPoint; // Parameter0
    Thread->Registers.OperatingMode = Subsystems[Process->Subsystem].OperatingMode;

    return STATUS_SUCCESS;
}

BOOLEAN KeCheckThread(THREAD* Thread) {
    if(!Thread) return FALSE;
    THREAD_LIST* l = Thread->ParentList;
    if(l->Magic != THREAD_LIST_MAGIC ||
    Thread->ParentListIndex > 63 ||
    Thread != &l->Threads[Thread->ParentListIndex]
    ) return FALSE;
    
    return TRUE;
}