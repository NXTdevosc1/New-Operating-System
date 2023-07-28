#include <ahci.h>

SPINLOCK _AhcAllocateSpinLock = 0;

UINT32 AhciAllocateSlot(PAHCIPORT Port) {
    UINT32 Ret;
    for(;;) {
        _BitScanForward(&Ret, Port->PendingCmd);
        if(!_bittestandset(&Port->PendingCmd, Ret)) return Ret;

        // TODO : Use events to sleep the thread better and provide cpu usage efficiency

        __Schedule();
    }
}

NSTATUS AhciIssueCommandSync(PAHCIPORT Port, UINT Cmd) {
    
}

