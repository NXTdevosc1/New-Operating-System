#include <ahci.h>

PVOID AhciAllocate(PAHCI Ahci, UINT64 Pages, UINT PageAttributes) {
    PVOID Ret = MmAllocateMemory(NULL, Pages, PageAttributes | (Ahci->LongAddresses ? 0 : PAGE_HALFPTR) | PAGE_WRITE_ACCESS, PAGE_CACHE_DISABLE);
    if(!Ret) AhciAbort(Ahci, STATUS_OUT_OF_MEMORY);

    if(PageAttributes & PAGE_2MB) {
        ZeroMemory(Ret, Pages << 21);
    } else ZeroMemory(Ret, Pages << 12);

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
        Ahci->HbaPorts[i].CommandStatus &= ~PORTxCMDxSTART;
        while(Ahci->HbaPorts[i].CommandStatus & PORTxCMDxCR) _mm_pause();
        Ahci->HbaPorts[i].CommandStatus &= ~PORTxCMDxFRE;
        while(Ahci->HbaPorts[i].CommandStatus & PORTxCMDxFRR) _mm_pause();
    }
    // Reset the HBA
    Ahci->Hba->Ghc |= HBA_GHC_RESET;
    while(Ahci->Hba->Ghc & HBA_GHC_RESET) Stall(10);
    Ahci->Hba->Ghc = HBA_GHC_AHCI_ENABLE;

    KDebugPrint("AHCI Resetted successfully");

}