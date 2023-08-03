#include <ahci.h>

NSTATUS __cdecl AhciInterruptHandler(INTERRUPT_HANDLER_DATA* Interrupt) {
    PAHCI Ahci = Interrupt->Context;
    UINT32 GlobalInterruptStatus = Ahci->Hba->InterruptStatus;
    if(!GlobalInterruptStatus) return STATUS_SUCCESS;
    KDebugPrint("AHCI Interrupt GIS %x", GlobalInterruptStatus);
    // Reset interrupt status
    ULONG Index;
    UINT32 gi = GlobalInterruptStatus;
    while(_BitScanForward(&Index, gi)) {
        _bittestandreset64(&gi, Index);
        PAHCIPORT Port = Ahci->Ports + Index;
        HBA_PORT* hbp = Port->HbaPort;
        KDebugPrint("AHCI : Interrupt on port %d IS %x 64Bit %x", Index, hbp->InterruptStatus, Ahci->LongAddresses);
        if(hbp->InterruptStatus.D2HRegisterFisInterrupt) {
            Port->FirstD2H = 1;
            KDebugPrint("AHCI : D2H Reg FIS SA %x CI %x", hbp->SataActive, hbp->CommandIssue);

            DWORD CommandIssue = Port->HbaPort->CommandIssue;
            DWORD Pending = Port->PendingCmd;
            ULONG CmdIndex;
            while(_BitScanForward(&CmdIndex, Pending)) {
                _bittestandreset(&Pending, CmdIndex);
                if(!_bittest(&CommandIssue, CmdIndex)) {
                    KDebugPrint("Command #%u Compeleted.", CmdIndex);
                    _interlockedbittestandreset(&Port->PendingCmd, CmdIndex);
                    if(Port->Commands[CmdIndex].Async) {
                        _InterlockedIncrement64(Port->Commands[CmdIndex].ReturnSet.IncrementOnDone);
                        _interlockedbittestandreset(&Port->AllocatedCmd, CmdIndex);
                    } else {
                        *Port->Commands[CmdIndex].ReturnSet.ReturnCode = STATUS_SUCCESS;
                        PETHREAD Thread = Port->Commands[CmdIndex].Thread;

                        while(!(KeGetThreadFlags(Thread) & (1 << THREAD_SUSPENDED))) _mm_pause();
                        // The thread should free the cmd slot after resuming
                        KeResumeThread(Thread);
                    }
                }
            }
            // hbp->InterruptStatus.D2HRegisterFisInterrupt = 1;
        } 
        if(hbp->InterruptStatus.PioSetupFisInterrupt) {
            KDebugPrint("AHCI : PIO Setup FIS");
            // hbp->InterruptStatus.PioSetupFisInterrupt = 1;
        }
        if(hbp->InterruptStatus.PhyRdyChangeStatus) {
            KDebugPrint("AHCI : PHYRDI Change");
            Port->HbaPort->SataError.PhyRdyChange = 1;
            Port->FirstD2H = 1;
        }
        if(hbp->InterruptStatus.PortConnectChangeStatus) {
            KDebugPrint("AHCI : PORT_CONNECT_CHANGE");
            Port->HbaPort->SataError.Exchanged = 1;
            hbp->SataControl.DeviceDetectionInitialization = 0;
        }
        if(hbp->InterruptStatus.TaskFileErrorStatus) {
            KDebugPrint("AHCI : TFERR");
            // hbp->InterruptStatus.TaskFileErrorStatus = 1;
        }
        if(hbp->InterruptStatus.UnknownFisInterrupt) {
            KDebugPrint("AHCI: UNKNOWN FIS");
            hbp->SataError.UnknownFisType = 0;
        }
        *(volatile UINT32*)&hbp->InterruptStatus = *(volatile UINT32*)&hbp->InterruptStatus;
    }
    KDebugPrint("AHCI Interrupt handler exit.");
    Ahci->Hba->InterruptStatus = GlobalInterruptStatus;

    return STATUS_SUCCESS;
}