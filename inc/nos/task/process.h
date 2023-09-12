#pragma once
#include <nosdef.h>
#include <nos/mm/mm.h>
#include <nos/ob/ob.h>
#include <ktask.h>

#define SUBSYSTEM_NATIVE 1
#define SUBSYSTEM_USERMODE_CONSOLE 3

typedef enum _OPERATING_MODE
{
    KERNEL_MODE,
    USER_MODE
} OPERATING_MODE;

typedef struct _SUBSYSTEM_DESCRIPTOR
{
    volatile UINT64 Flags;
    BOOLEAN OperatingMode;
    SUBSYSTEM_ENTRY_POINT EntryPoint;
    UINT64 NumPages; // If user mode, then the caller should specify number of pages in the entry point
} SUBSYSTEM_DESCRIPTOR;
SUBSYSTEM_DESCRIPTOR Subsystems[];
typedef struct _REGISTER_BLOCK
{
    char XsaveRegion[0x1008]; // additionnal 8 bytes for alignment
    // Interrupt stack registers
    UINT64 rip, cs, rflags, rsp, ss;
    // general purpose registers
    UINT64 rax, rbx, rcx, rdx, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15, rbp;
    // Page table
    UINT64 Cr3;
    // other segments (Packed in one 64 Bit value)
    UINT16 ds, es, fs, gs;

} REGISTER_BLOCK;

// Current MAX is 4096 Thread per CPU
typedef volatile struct _THREAD_QUEUE_ENTRY
{
    PETHREAD Thread;
    volatile struct _THREAD_QUEUE_ENTRY *Previous;
    volatile struct _THREAD_QUEUE_ENTRY *Next;

} THREAD_QUEUE_ENTRY, *PTHREAD_QUEUE_ENTRY;

typedef struct _PROCESSOR PROCESSOR, *RFPROCESSOR;

#define GetThreadFlag(Thread, Flag) (_bittest64(&Thread->Flags, Flag))
#define SetThreadFlag(Thread, Flag) (_bittestandset64(&Thread->Flags, Flag))
#define ResetThreadFlag(Thread, Flag) (_bittestandreset64(&Thread->Flags, Flag))

typedef volatile struct _ETHREAD
{
    // Those are basic data used by scheduler
    REGISTER_BLOCK Registers;
    PEPROCESS Process;
    UINT64 ThreadId;
    UINT64 StaticPriority;
    UINT64 DynamicPriority;
    // Data used by other system components
    UINT64 Flags;
    NSTATUS Status; // New feature
    NSTATUS ExitCode;
    PETHREAD NextThread;

    POBJECT ThreadObject;

    THREAD_QUEUE_ENTRY QueueEntry;

    struct
    {
        UINT64 TicksSinceBoot; // How many seconds
        UINT64 CounterValue;   // depends on the frequency of the counter
    } SleepUntil;

    RFPROCESSOR Processor;

    void *ParentList;
    UINT ParentListIndex;

    UINT64 CallbackStatus;
    UINT64 IoParameters[0x100];
    PDRIVER RunningDriver; // Driver occupying this threads cpu time (Set for example when performing Synchronous IO or running driver's entry point on the thread)
    struct
    {
        PDEVICE IoDevice;
        UINT Function;
    } RunningIo;
    PVOID StackMem;
    UINT64 StackPages;
} ETHREAD, *PETHREAD;

typedef enum
{
    PROCESS_CONTROL_LINK_THREAD = 0,
    PROCESS_CONTROL_MANAGE_ADDRESS_SPACE,
} PROCESS_CONTROL_BITMASK;

// TODO : SpinLock (disable Interrupts)
#define ProcessAcquireControlLock(_Process, _ControlBit)                            \
    {                                                                               \
        KDebugPrint("Acquire CTRL BIT %u PROCESS %x", _ControlBit, _Process);       \
        while (_interlockedbittestandset64(&_Process->ControlBitmask, _ControlBit)) \
        {                                                                           \
            _mm_pause();                                                            \
        }                                                                           \
    }

#define ProcessReleaseControlLock(_Process, _ControlBit)                  \
    KDebugPrint("Release CTRL BIT %d PROCESS %x", _ControlBit, _Process); \
    _interlockedbittestandreset64(&_Process->ControlBitmask, _ControlBit)

#define ProcessOperatingMode(_Process) (Subsystems[_Process->Subsystem].OperatingMode)

// PROCESS Flags
#define PROCESS_FLAG_STARTEDUP 0x100 // If not set then load DLLs first

typedef volatile struct _EPROCESS
{
    PEPROCESS Parent;
    UINT64 ProcessId;
    UINT Priority;
    UINT64 Flags;
    UINT64 ControlBitmask; // Mutexes to modify the process
    UINT16 *ProcessDisplayName;
    UINT16 *Path;
    UINT16 *ProcessDescription;
    void *PageTable;
    UINT8 Subsystem;

    volatile UINT64 NumberOfThreads;
    volatile UINT64 NumberOfChildProcesses;

    PEPROCESS NextChild; // continuation of the child array for this child process

    POBJECT ProcessObject;

    // Memory Management
    void *VmSearchStart;
    void *VmSearchEnd;

} EPROCESS, *PEPROCESS;

/*
- Multi-Operating-Mode Function
- Runtime checks and pointer conversion
- Call to ExCreateProcess
*/
NSTATUS NSYSAPI NosCreateProcess(
    IN OPT HANDLE Parent,
    OUT OPT UINT64 *ProcessId,
    IN UINT64 Flags,
    IN UINT8 Subsystem,
    IN UINT16 *DisplayName,
    IN UINT16 *Path,
    IN void *EntryPoint);

/*
- Multi-Operating-Mode Function
- Runtime checks and pointer conversion
- Call to ExCreateThread
*/
NSTATUS NSYSAPI NosCreateThread(
    IN UINT64 Flags,
    IN void *EntryPoint,
    OUT OPT HANDLE *Thread);

void KiInitMultitaskingSubsystem();