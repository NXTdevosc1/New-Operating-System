#include <nos/nos.h>
#include <nos/task/process.h>
#include <nos/processor/hw.h>
#include <nos/mm/mm.h>
#include <kmem.h>

NSTATUS ThreadEvtHandler(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access)
{
    return STATUS_SUCCESS;
}

static UINT64 LastCreateProcessorId = 0;
static SPINLOCK _SelectPidSpinlock = 0;
NSTATUS KRNLAPI KeCreateThread(
    IN PEPROCESS Process,
    OUT OPT PETHREAD *OutThread,
    IN UINT64 Flags,
    IN void *EntryPoint,
    IN void *Context)
{
    if (!Process)
        Process = KernelProcess;
    if (!KeProcessExists(Process))
    {
        KDebugPrint("KECREATETHREAD PROCESS %x DOES NOT EXIST", Process);
        return STATUS_INVALID_PARAMETER;
    }
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
        ThreadEvtHandler);

    if (NERROR(s))
    {
        KDebugPrint("failed to create object thread");
        while (1)
            __halt();
    }

    Thread = Ob->Address;

    if (!BootProcessor->InternalData->CurrentThread)
    {
        // This is the kernel thread
        BootProcessor->InternalData->CurrentThread = Thread;
        BootProcessor->InternalData->IdleThread = Thread;
        // the kernel will remove the main thread from the priority list when it finishes initialization
    }

    // Setting up the thread
    Thread->ThreadObject = Ob;
    Thread->Process = Process;
    Thread->ThreadId = Ob->ObjectId;
    Thread->Flags = Flags;
    Thread->QueueEntry.Thread = Thread;

    // TODO : Select processor based on load balance
    UINT64 cpf = ExAcquireSpinLock(&_SelectPidSpinlock);
    Thread->Processor = KeGetProcessorByIndex(LastCreateProcessorId);
    LastCreateProcessorId++;
    // >= instead of == to handle case before any processor is registred
    if (LastCreateProcessorId == NumProcessors)
        LastCreateProcessorId = 0;
    ExReleaseSpinLock(&_SelectPidSpinlock, cpf);
    if (!Thread->Processor)
    {
        KDebugPrint("Failed to get thread processor");
        while (1)
            __halt();
    }

    _InterlockedIncrement64(&Process->NumberOfThreads);

    if (OutThread)
        *OutThread = Thread;

    // Setup thread registers
    Thread->Registers.rip = (UINT64)Subsystems[Process->Subsystem].EntryPoint;
    Thread->Registers.rcx = (UINT64)EntryPoint; // Parameter0
    Thread->Registers.rdx = (UINT64)Context;

    Thread->Registers.Cr3 = (UINT64)Process->PageTable;
    // Setup segments
    if (Subsystems[Process->Subsystem].OperatingMode == KERNEL_MODE)
    {
        Thread->Registers.cs = 0x08;
        Thread->Registers.ds = 0x10;
        Thread->Registers.es = 0x10;
        Thread->Registers.fs = 0x10;
        Thread->Registers.gs = 0x10;
        Thread->Registers.ss = 0x10;
    }
    else
    {
        KDebugPrint("User mode thread !");
        while (1)
            ;
    }
    KDebugPrint("System Create Thread PID=%u TID=%u", Process->ProcessId, Thread->ThreadId);

    Thread->Registers.rflags = 0x200; // Interrupt enable
    // Allocate the stack (TODO : Use a dynamic stack)

    PVOID Stack = KRequestMemory(REQUEST_VIRTUAL_MEMORY, Process, MEM_COMMIT, 0x10);
    Thread->StackMem = Stack;
    Thread->StackPages = 0x10;

    Thread->Registers.rsp = (UINT64)Stack + 0xE008;
    Thread->Registers.rbp = (UINT64)Stack + 0x10000;

    Thread->RunningDriver = KeGetCurrentThread()->RunningDriver;

    if (!(Flags & THREAD_CREATE_IDLE))
    {
        KeSetThreadPriority(Thread, THREAD_PRIORITY_NORMAL);
    }
    return STATUS_SUCCESS;
}

BOOLEAN KRNLAPI KeThreadExists(PETHREAD Thread)
{
    UINT64 f = PAGE_WRITE_ACCESS;
    if (!Thread || !KeCheckMemoryAccess(KernelProcess, (void *)Thread, sizeof(ETHREAD), &f))
        return FALSE;
    if (!ObCheckObject(Thread->ThreadObject) || Thread->ThreadObject->ObjectType != OBJECT_THREAD ||
        Thread->ThreadObject->Address != Thread)
        return FALSE;

    return TRUE;
}

#include <nos/ob/obutil.h>

// Finds the thread and returns raw pointer
PETHREAD KRNLAPI KeGetThreadById(UINT64 ThreadId)
{
    HANDLE h;
    if (NERROR(ObOpenHandleById(KernelProcess, OBJECT_THREAD, ThreadId, 0, &h)))
        return NULL;
    PETHREAD t = ObiReferenceByHandle(h)->Object->Address;
    ObCloseHandle(KernelProcess, h);
    return t;
}

PETHREAD KRNLAPI KeWalkThreads(PEPROCESS Process, UINT64 *EnumVal)
{
    POBJECT obj;
    *EnumVal = ObEnumerateObjects(Process->ProcessObject, OBJECT_THREAD, &obj, NULL, *EnumVal);
    if (!(*EnumVal))
        return NULL;

    return obj->Address;
}

BOOLEAN KRNLAPI KeSetThreadPriority(PETHREAD Thread, UINT ThreadPriority)
{
    if (!KeThreadExists(Thread) || ThreadPriority > THREAD_PRIORTIY_REALTIME)
        return FALSE;

    return KeSetStaticPriority(Thread, ThreadPriority + 5 * Thread->Process->Priority);
}

NSTATUS KiSetThreadInReadyQueue(PETHREAD Thread)
{
    // KDebugPrint("Remote sclink processorid %u", KeGetCurrentProcessorId());
    ScLinkReadyThreadBottom(&Thread->QueueEntry);
    return STATUS_SUCCESS;
}

// Or you can manually set a priority of your own
BOOLEAN KRNLAPI KeSetStaticPriority(PETHREAD Thread, UINT StaticPriority)
{
    Thread->StaticPriority = StaticPriority;
    if (Thread->DynamicPriority < StaticPriority)
        Thread->DynamicPriority = StaticPriority;

    // TODO : Send system interrupt
    if (!GetThreadFlag(Thread, THREAD_READY) && !GetThreadFlag(Thread, THREAD_SUSPENDED))
    {
        if (Thread->Processor == KeGetCurrentProcessor())
        {
            UINT64 rf = __readeflags();
            _disable();
            ScLinkReadyThreadBottom(&Thread->QueueEntry);
            __writeeflags(rf);
        }
        else
        {
            // KDebugPrint("Set static priority, remote execute thread id %u processor %u", Thread->ThreadId, Thread->Processor->Id.ProcessorId);
            KeRemoteExecute(Thread->Processor, KiSetThreadInReadyQueue, (void *)Thread, FALSE);
        }
    }
    return TRUE;
}

PETHREAD KRNLAPI KeGetCurrentThread()
{
    PETHREAD T = KeGetCurrentProcessor()->InternalData->CurrentThread;
    if (!T)
    {
        SerialLog("FATAL_ERROR : GetCurrentThread() == NULL");
        KeRaiseException(STATUS_FATAL_ERROR);
    }
    return T;
}

UINT64 KRNLAPI KeGetCurrentThreadId()
{
    return KeGetCurrentThread()->ThreadId;
}

void KRNLAPI KeSetThreadStatus(NSTATUS Status)
{
    KeGetCurrentThread()->Status = Status;
}

NSTATUS KRNLAPI KeGetThreadStatus()
{
    return KeGetCurrentThread()->Status;
}

void __declspec(noreturn) KRNLAPI KeRaiseException(NSTATUS ExceptionCode)
{
    KDebugPrint("System Raise Exception : ExceptionCode %d", ExceptionCode);
    PETHREAD Thread = KeGetCurrentThread();
    KeSetThreadStatus(ExceptionCode);
    KDebugPrint("System Raise Exception : ThreadID %d ProcessId %d ProcessorId %d", Thread->ThreadId, Thread->Process->ProcessId, Thread->Processor->Id.ProcessorId);
    while (1)
        __halt();
}

void KRNLAPI KeSuspendThread()
{
    _disable();
    PETHREAD Thread = KeGetCurrentThread();
    ScUnlinkReadyThread(&Thread->QueueEntry);
    SetThreadFlag(Thread, THREAD_SUSPENDED);
    KDebugPrint("Suspended thread#%u", Thread->ThreadId);
    __Schedule();
    KDebugPrint("thread#%u back to work", Thread->ThreadId);
}

// System Interrupt handler
NSTATUS KiThreadResumeRoutine(PETHREAD Thread)
{
    ResetThreadFlag(Thread, THREAD_SUSPENDED);
    ScLinkReadyThreadBottom(&Thread->QueueEntry);
    KDebugPrint("Thread #%u Resumed", Thread->ThreadId);
    return STATUS_SUCCESS;
}

void KRNLAPI KeResumeThread(PETHREAD Thread)
{
    PROCESSOR *Processor = KeGetCurrentProcessor();
    if (!GetThreadFlag(Thread, THREAD_SUSPENDED))
        return;
    if (Processor != Thread->Processor)
    {
        KDebugPrint("Resuming thread from an alternate processor, sending IPI");
        // Send internal interrupt (without waiting)
        KeRemoteExecute(Thread->Processor, KiThreadResumeRoutine, (void *)Thread, FALSE);
    }
    else
    {
        UINT64 f = __readeflags();
        _disable();
        KiThreadResumeRoutine(Thread);
        __writeeflags(f);
    }
}

UINT64 KRNLAPI KeGetThreadFlags(PETHREAD Thread)
{
    KDebugPrint("Get th flags %x", Thread->Flags);
    return Thread->Flags;
}
