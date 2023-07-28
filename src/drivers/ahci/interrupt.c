#include <ahci.h>

NSTATUS __cdecl AhciInterruptHandler(INTERRUPT_HANDLER_DATA* Interrupt) {
    PAHCI Ahci = Interrupt->Context;
    UINT32 GlobalInterruptStatus = Ahci->Hba->InterruptStatus;
    if(!GlobalInterruptStatus) return STATUS_SUCCESS;

    // Reset interrupt status
    ULONG Index;
    UINT32 gi = GlobalInterruptStatus;
    while(_BitScanForward(&Index, gi)) {
        _bittestandreset64(&gi, Index);
        PAHCIPORT Port = Ahci->Ports + Index;
        HBA_PORT* hbp = Port->HbaPort;
        KDebugPrint("AHCI : Interrupt on port %d", Index);
        if(hbp->InterruptStatus.D2HRegisterFisInterrupt) {
            Port->FirstD2H = 1;

            KDebugPrint("AHCI : D2H Reg FIS");
        } 
        if(hbp->InterruptStatus.PioSetupFisInterrupt) {
            KDebugPrint("AHCI : PIO Setup FIS");
        }
        if(hbp->InterruptStatus.PhyRdyChangeStatus) {
            KDebugPrint("AHCI : PHYRDI Change");
            Port->HbaPort->SataError.PhyRdyChange = 1;
            Port->FirstD2H = 1;
        }
        if(hbp->InterruptStatus.PortConnectChangeStatus) {
            KDebugPrint("AHCI : PORT_CONNECT_CHANGE");
            Port->HbaPort->SataError.Exchanged = 1;
        }
        *(volatile UINT32*)&hbp->InterruptStatus = *(volatile UINT32*)&hbp->InterruptStatus;
    }
    Ahci->Hba->InterruptStatus = GlobalInterruptStatus;
    return STATUS_SUCCESS;
}