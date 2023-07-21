#include <ahci.h>

PVOID AhciAllocate(PAHCI Ahci, UINT64 Pages, UINT PageAttributes) {
    PVOID Ret = MmAllocateMemory(NULL, Pages, PageAttributes | (Ahci->LongAddresses ? 0 : PAGE_HALFPTR));
    if(!Ret) AhciAbort(Ahci, STATUS_OUT_OF_MEMORY);

    return Ret;
}

void AhciAbort(PAHCI Ahci, NSTATUS ExitCode) {
    KDebugPrint("Ahci %x Exited with code %d", Ahci, ExitCode);
    while(1) __halt();
}

void AhciReset(PAHCI Ahci) {
    // Turn off all the ports
    KDebugPrint("Resetting AHC %x", Ahci);
    DWORD pi = Ahci->Hba->PortsImplemented;
    ULONG i;
    while(_BitScanForward(&i, pi)){
        _bittestandreset(&pi, i);
        Ahci->Ports[i].CommandStatus &= ~PORTxCMDxSTART;
        while(Ahci->Ports[i].CommandStatus & PORTxCMDxCR) _mm_pause();
        Ahci->Ports[i].CommandStatus &= ~PORTxCMDxFRE;
        while(Ahci->Ports[i].CommandStatus & PORTxCMDxFRR) _mm_pause();
    }
    // Reset the HBA
    Ahci->Hba->GlobalHostControl.HbaReset = 1;
    while(Ahci->Hba->GlobalHostControl.HbaReset) _mm_pause();
    Ahci->Hba->GlobalHostControl.AhciEnable = 1;

    KDebugPrint("AHCI Resetted successfully");

}