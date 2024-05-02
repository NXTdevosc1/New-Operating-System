#include <nos/nos.h>
#include <nos/task/process.h>

SUBSYSTEM_DESCRIPTOR Subsystems[0x100] = {0};

NSTATUS ProcessEvt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access)
{
    return STATUS_SUCCESS;
}
BOOLEAN PreOsExtendedSpaceBoolean = TRUE;
NSTATUS KRNLAPI KeCreateProcess(
    IN OPT PEPROCESS ParentProcess,
    OUT PEPROCESS *OutProcess,
    IN UINT64 CreateFlags,
    IN UINT8 Subsystem,
    IN UINT16 *DisplayName,
    IN UINT16 *Path,
    IN void *EntryPoint)
{
    // Runtime Checks
    if (!Subsystems[Subsystem].Flags)
        return STATUS_SUBSYSTEM_NOT_PRESENT;
    POBJECT ObParent = NULL;
    POBJECT Ob;
    if (ParentProcess)
    {
        if (!KeProcessExists(ParentProcess))
            return STATUS_INVALID_PARAMETER;
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
        ProcessEvt);

    if (NERROR(s))
    {
        KDebugPrint("failed to create object process status : %d", s);
        while (1)
            __halt();
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
    Process->Priority = PROCESS_PRIORITY_NORMAL;

    if (OutProcess)
        *OutProcess = Process;

    if (Process->Subsystem == SUBSYSTEM_NATIVE)
    {
        // Kernel mode process
        Process->VmSearchStart = (PVOID)0xFFFFF00000000000;
        Process->VmSearchEnd = (void *)KeReserveExtendedSpace(1);
        Process->PageTable = GetCurrentPageTable();
        if (*OutProcess == KernelProcess)
        {
            PreOsExtendedSpaceBoolean = FALSE;
        }
        else
        {
            // Another kernel mode process
            Process->VmImage = KernelProcess->VmImage;
            goto KmodeSkipVmSetup;
        }
    }
    else
    {
        Process->VmSearchStart = (PVOID)0x1000;
        Process->VmSearchEnd = (PVOID)0x800000000000;
    }
    InitVirtualMemoryManager(Process);
KmodeSkipVmSetup:
    KDebugPrint("VMSETUP");
    // Create main thread
    UINT ThreadCreateFlags = 0;
    if ((CreateFlags & PROCESS_CREATE_IDLE))
        ThreadCreateFlags |= THREAD_CREATE_IDLE;
    // NULL Context because entry point has its own context predefined by the subsystem
    s = KeCreateThread(Process, NULL, ThreadCreateFlags, EntryPoint, NULL);
    if (NERROR(s))
    {
        // TODO : Remove process
        KDebugPrint("createprocess : failed to create thread status : %d", s);
        while (1)
            ;
    }

    return STATUS_SUCCESS;
}

#include <nos/ob/obutil.h>

BOOLEAN KeProcessExists(PEPROCESS Process)
{
    UINT64 f = PAGE_WRITE_ACCESS;

    if (!Process || !KeCheckMemoryAccess(KernelProcess, (void *)Process, sizeof(EPROCESS), &f))
        return FALSE;
    if (!ObCheckObject(Process->ProcessObject) || Process->ProcessObject->ObjectType != OBJECT_PROCESS ||
        Process->ProcessObject->Address != Process)
        return FALSE;

    return TRUE;
}

// Finds the process and returns raw pointer
PEPROCESS KeGetProcessById(UINT64 ProcessId)
{
    HANDLE h;
    if (NERROR(ObOpenHandleById(KernelProcess, OBJECT_PROCESS, ProcessId, 0, &h)))
        return NULL;
    PEPROCESS p = ObiReferenceByHandle(h)->Object->Address;
    ObCloseHandle(KernelProcess, h);
    return p;
}

NSTATUS KRNLAPI KeAcquireControlFlag(IN PEPROCESS Process, IN UINT64 ControlBit)
{
    if (ControlBit > 63)
        return STATUS_INVALID_PARAMETER;
    if (!Process)
        Process = KernelProcess;
    KDebugPrint("Acquire CTRL BIT %u PROCESS %x", ControlBit, Process);
    while (_interlockedbittestandset64(&Process->ControlBitmask, ControlBit))
        _mm_pause();

    return STATUS_SUCCESS;
}
NSTATUS KRNLAPI KeReleaseControlFlag(IN PEPROCESS Process, IN UINT64 ControlBit)
{
    if (ControlBit > 63)
        return STATUS_INVALID_PARAMETER;
    if (!Process)
        Process = KernelProcess;
    KDebugPrint("Release CTRL BIT %d PROCESS %x", ControlBit, Process);
    _interlockedbittestandreset64(&Process->ControlBitmask, ControlBit);
    return STATUS_SUCCESS;
}

void KiInitMultitaskingSubsystem()
{
    if (NERROR(KeCreateProcess(NULL,
                               &KernelProcess,
                               0,
                               SUBSYSTEM_NATIVE,
                               L"NOS System.",
                               L"//NewOS/System/noskx64.exe",
                               NULL)))
        RaiseInitError(0);
    KDebugPrint("MS");
}

BOOLEAN KRNLAPI KeSetProcessPriority(PEPROCESS Process, UINT Priority)
{
    if (Priority > 5 && Priority != PROCESS_PRIORITY_CONSTANT_EXECUTION)
        return FALSE;
    PETHREAD Thread;
    UINT64 ev;
    Process->Priority = Priority;
    while ((Thread = KeWalkThreads(Process, &ev)))
    {
        Thread->StaticPriority = (Thread->StaticPriority % 5) * (Priority / 5);
    }
    return TRUE;
}

PEPROCESS KRNLAPI KeGetCurrentProcess()
{
    return KeGetCurrentProcessor()->InternalData->CurrentThread->Process;
}

UINT64 KRNLAPI KeGetCurrentProcessId()
{
    return KeGetCurrentProcess()->ProcessId;
}