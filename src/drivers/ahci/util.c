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
    // Stop all the ports
    while(_BitScanForward(&i, pi)){
        _bittestandreset(&pi, i);
        Ahci->Ports[i].CommandStatus &= ~PORTxCMDxSTART;
        while(Ahci->Ports[i].CommandStatus & PORTxCMDxCR) _mm_pause();
        Ahci->Ports[i].CommandStatus &= ~PORTxCMDxFRE;
        while(Ahci->Ports[i].CommandStatus & PORTxCMDxFRR) _mm_pause();
    }
    // Reset the HBA
    Ahci->Hba->Ghc |= HBA_GHC_RESET;
    while(Ahci->Hba->Ghc & HBA_GHC_RESET) Stall(10);
    Ahci->Hba->Ghc = HBA_GHC_AHCI_ENABLE;

    KDebugPrint("AHCI Resetted successfully");

}