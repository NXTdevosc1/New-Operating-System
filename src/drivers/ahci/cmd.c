#include <ahci.h>


UINT32 AhciAllocateSlot(PAHCIPORT Port) {
    UINT32 Ret;
    for(;;) {
        _BitScanForward(&Ret, ~Port->AllocatedCmd);
        if(!_interlockedbittestandset(&Port->AllocatedCmd, Ret)) return Ret;

        // TODO : Use events to sleep the thread better and provide cpu usage efficiency

        if(Port->AllocatedCmd == (UINT32)-1) __Schedule();
    }
}

NSTATUS AhciIssueCommandSync(PAHCIPORT Port, UINT Cmd) {

    // Everything is uncached


    _disable(); // The operation should be done fast without scheduling

    NSTATUS Ret;

    Port->Commands[Cmd].Thread = KeGetCurrentThread();
    Port->Commands[Cmd].ReturnSet.ReturnCode = &Ret;


    // Command issue should be set on the port before pending cmd
    _interlockedbittestandset(&Port->HbaPort->CommandIssue, Cmd);
    _interlockedbittestandset(&Port->PendingCmd, Cmd);

    KDebugPrint("Issuing command#%u", Cmd);

    // Interrupt handler should wait the thread until it suspends before it resumes it
    while(_bittest(&Port->PendingCmd, Cmd)) KeSuspendThread();
    // Suspend thread will automatically re-enable interrupts
    // Slots are automatically freed
    
    _interlockedbittestandreset64(&Port->AllocatedCmd, Cmd);
    
    KDebugPrint("AHCI Command#%u issued successfully SATA_ACTIVE %x CI %x", Cmd, Port->HbaPort->SataActive, Port->HbaPort->CommandIssue);

    return Ret;
}


// Used to help performing multiple commands at once
void AhciIssueCommandAsync(PAHCIPORT Port, UINT Cmd, UINT64* IncrementOnDone) {
    Port->Commands[Cmd].Async = TRUE;
    Port->Commands[Cmd].ReturnSet.IncrementOnDone = IncrementOnDone;

    // Command issue should be set on the port before pending cmd
    _interlockedbittestandset(&Port->HbaPort->CommandIssue, Cmd);
    _interlockedbittestandset(&Port->PendingCmd, Cmd);

    // Allocate cmd automatically reset on async operations
}