#include <nos/nos.h>
#include <nos/task/schedule.h>

void KRNLAPI KeExitThread(PETHREAD Thread, NSTATUS ExitCode) {
    if(!Thread) Thread = KeGetCurrentThread();
    
    // KDebugPrint("Exitting thread #%u", Thread->ThreadId);
    _disable(); // Avoid task switch



    if(GetThreadFlag(Thread, THREAD_READY)) {
        // Remove thread from ready queue
        ScUnlinkReadyThread(&Thread->QueueEntry);
    }

    // Free all thread's memory
    if(!MmFreeMemory(Thread->Process, Thread->StackMem, Thread->StackPages)) {
        KDebugPrint("EXIT_THREAD BUG0");
        while(1) __halt(); 
    }

    ProcessAcquireControlLock(Thread->Process, PROCESS_CONTROL_LINK_THREAD);


    _InterlockedDecrement64(&Thread->Process->NumberOfThreads);

    if(!Thread->Process->NumberOfThreads) {
        // Exit the whole process
        KDebugPrint("EXIT_THREAD EXIT_PROCESS");
        while(1) __halt();
    }

    ProcessReleaseControlLock(Thread->Process, PROCESS_CONTROL_LINK_THREAD);

    Thread->ExitCode = ExitCode;

    // KDebugPrint("Thread #%u exited with code %d", Thread->ThreadId, ExitCode);
    
    // Destroy the thread object
    if(!ObDestroyObject(Thread->ThreadObject, FALSE)) {
        KDebugPrint("EXIT_THREAD BUG1");
        while(1) __halt(); 
    }

    // If the host thread called exit then it wont schedule back
    // Otherwise we just continue execution :)
    __Schedule();
}