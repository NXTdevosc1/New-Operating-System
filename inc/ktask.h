#pragma once
#include <nosdef.h>

// Process Create Flags
#define PROCESS_CREATE_IDLE 0x80 // Sends THREAD_CREATE_IDLE Flag when creating the main thread

typedef struct _EXECUTABLE_IMAGE_DESCRIPTOR {
    BOOLEAN Imported;
    void* PeHeader;
    void* EntryPoint;
    void* VirtualBase;
    UINT64 VirtualSize;
} EXECUTABLE_IMAGE_DESCRIPTOR, *PEXECUTABLE_IMAGE_DESCRIPTOR;

/*
- Core function that creates a process
- There are no runtime checks and the function should be called in kernel-mode only
*/
NSTATUS KRNLAPI KeCreateProcess(
    IN OPT PEPROCESS ParentProcess,
    OUT PEPROCESS* OutProcess,
    IN UINT64 CreateFlags,
    IN UINT8 Subsystem,
    IN UINT16* DisplayName,
    IN UINT16* Path,
    IN void* EntryPoint
);

// THREAD PRIORITY
#define THREAD_PRIORITY_VERY_LOW 0
#define THREAD_PRIORITY_LOW 1
#define THREAD_PRIORITY_NORMAL 2
#define THREAD_PRIORITY_HIGH 3
#define THREAD_PRIORTIY_REALTIME 4
// PROCESS PRIORITY
#define PROCESS_PRIORITY_LOW 0
#define PROCESS_PRIORITY_BELOW_NORMAL 1
#define PROCESS_PRIORITY_NORMAL 2
#define PROCESS_PRIORITY_ABOVE_NORMAL 3
#define PROCESS_PRIORITY_HIGH 4
#define PROCESS_PRIORITY_VERY_HIGH 5
#define PROCESS_PRIORITY_CONSTANT_EXECUTION 100

// Thread Create Flags
#define THREAD_CREATE_IDLE 1 // Do not put the thread in the ready queue yet
/*
- Core function that creates a thread
- There are no runtime checks and the function should be called in kernel-mode only
*/
NSTATUS KRNLAPI KeCreateThread(
    IN PEPROCESS Process,
    OUT PETHREAD* OutThread,
    IN UINT64 Flags,
    IN void* EntryPoint,
    IN void* Context
);

typedef void (__cdecl *SUBSYSTEM_ENTRY_POINT)(void* EntryPoint, void* Context);


/*
- Enumerates the threads of a process
*/
PETHREAD KRNLAPI KeWalkThreads(PEPROCESS Process, UINT64* EnumVal);

/*
- This function can only be called by kernel/drivers/services
- Runtime checks
*/
NSTATUS NSYSAPI KeCreateSubsystem(
    IN UINT8 Subsystem,
    IN BOOLEAN OperatingMode,
    IN SUBSYSTEM_ENTRY_POINT EntryPoint,
    IN UINT64 NumPages // not required if OperatingMode=KERNEL_MODE
);


/*
- Checks if a process exists
*/
BOOLEAN KRNLAPI KeProcessExists(PEPROCESS Process);
/*
- Checks if a thread exists
*/
BOOLEAN KRNLAPI KeThreadExists(ETHREAD* ETHREAD);

/*
- Get raw pointer to process object by its id
*/
PEPROCESS KRNLAPI KeGetProcessById(UINT64 Process);
/*
- Get raw pointer to thread object by its id
*/
PETHREAD KRNLAPI KeGetThreadById(UINT64 ThreadId);

BOOLEAN KRNLAPI KeSetStaticPriority(PETHREAD Thread, UINT StaticPriority);
BOOLEAN KRNLAPI KeSetThreadPriority(PETHREAD Thread, UINT ThreadPriority);

PEPROCESS KRNLAPI KeGetCurrentProcess();
PETHREAD KRNLAPI KeGetCurrentThread();

UINT64 KRNLAPI KeGetCurrentProcessId();
UINT64 KRNLAPI KeGetCurrentThreadId();


BOOLEAN KRNLAPI KeSetProcessPriority(PEPROCESS Process, UINT Priority);

void KRNLAPI KeSetThreadStatus(NSTATUS Status);
NSTATUS KRNLAPI KeGetThreadStatus();

void __declspec(noreturn) KRNLAPI KeRaiseException(NSTATUS ExceptionCode);


void KRNLAPI KeExitThread(PETHREAD Thread, NSTATUS ExitCode);


void KRNLAPI __Schedule();

void KRNLAPI KeSuspendThread();
void KRNLAPI KeResumeThread(PETHREAD Thread);


// if wait, returns status of the remote execution routine
// otherwise it returns STATUS_SUCCESS
NSTATUS KRNLAPI KeRemoteExecute(
    IN PROCESSOR* Processor,
    IN REMOTE_EXECUTE_ROUTINE Routine,
    IN OPT void* Context,
    IN BOOLEAN Wait
);

// THREAD_FLAGS (Bit field)
typedef enum _THREAD_FLAGS {
    THREAD_READY,
    THREAD_SUSPENDED
} THREAD_FLAGS;

UINT64 KRNLAPI KeGetThreadFlags(PETHREAD Thread);