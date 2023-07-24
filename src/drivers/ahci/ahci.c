#include <ahci.h>

NSTATUS AhciInitDevice(PCI_DEVICE_LOCATION* ploc) {
    KDebugPrint("Found AHCI Controller at location %x", ploc);

    PAHCI Ahc = AllocateNullPool(sizeof(AHCI));
    if(!Ahc) return STATUS_OUT_OF_MEMORY;

    Ahc->Hba = PciGetBaseAddress(&Pci, ploc, 5);
    KeMapVirtualMemory(NULL, (void*)Ahc->Hba, (void*)Ahc->Hba, 3, PAGE_WRITE_ACCESS, PAGE_CACHE_DISABLE);

    KDebugPrint("AHCI Base Address %x BAR5 %x", Ahc->Hba, Pci.Read64(ploc, 4 + PCI_BAR + 5 * 4));

    // Reset the AHC
    // Fill the rest of the structure
    Ahc->HbaPorts = (void*)(((char*)Ahc->Hba) + AHCI_PORTS_OFFSET);

    Ahc->Device = KeCreateDevice(DEVICE_CONTROLLER, 0, L"Advanced Host Controller Interface (AHCI)", Ahc);
    if(!Ahc->Device) {
        AhciAbort(Ahc, STATUS_UNSUPPORTED);
    }
    if(Ahc->Hba->HostCapabilities.x64AddressingCapability) {
        Ahc->LongAddresses = TRUE;
    }

    // Setup the AHCI
    Ahc->Hba->Ghc = HBA_GHC_AHCI_ENABLE;
    if(Ahc->Hba->ExtendedHostCapabilities.BiosOsHandoff) {
    KDebugPrint("AHC Supports BIOS/OS Handoff OS %x BB %x BO %x", Ahc->Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore, Ahc->Hba->BiosOsHandoffControlAndStatus.BiosBusy, Ahc->Hba->BiosOsHandoffControlAndStatus.BiosOwnedSemaphore);
        Ahc->Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore = 1;
        while(Ahc->Hba->BiosOsHandoffControlAndStatus.BiosOwnedSemaphore);

        KDebugPrint("AHC Ownership taken OS %x BB %x BO %x", Ahc->Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore, Ahc->Hba->BiosOsHandoffControlAndStatus.BiosBusy, Ahc->Hba->BiosOsHandoffControlAndStatus.BiosOwnedSemaphore);
        
        Sleep(25);
        UINT Count = 41; // Wait for 2 seconds in 50ms chunks
        while(Count--) {
            if(!Ahc->Hba->BiosOsHandoffControlAndStatus.BiosBusy) break;
            Sleep(50);
        }
        
        if(Ahc->Hba->BiosOsHandoffControlAndStatus.BiosBusy) KDebugPrint("Warning, BIOS is still busy");
    }

    AhciReset(Ahc);
    Ahc->NumSlots = Ahc->Hba->HostCapabilities.NumCommandSlots + 1;

    KDebugPrint("AHCI Enabled successfully NUM_CMD_SLOTS %d", Ahc->NumSlots);
    
    // Enable MSI Interrupts
    EnableMsiInterrupts(&Pci, ploc, AhciInterruptHandler, Ahc);
    
    Ahc->Hba->Ghc |= HBA_GHC_INTENABLE;

    // Initialize each port
    DWORD pi = Ahc->Hba->PortsImplemented;
    ULONG iPort;
    while(_BitScanForward(&iPort, pi)) {
        _bittestandreset(&pi, iPort);
        HBA_PORT* HbaPort = Ahc->HbaPorts + iPort;
        KeMapVirtualMemory(NULL, (void*)HbaPort, (void*)HbaPort, 1, PAGE_WRITE_ACCESS, PAGE_CACHE_DISABLE);
        PAHCIPORT Port = Ahc->Ports + Ahc->NumPorts;
        Ahc->NumPorts++;
        
        // Initialize port values
        Port->Ahci = Ahc;
        Port->PortIndex = iPort;
        Port->HbaPort = HbaPort;
        
        AhciInitPort(Port);

    }
    return STATUS_SUCCESS;
}