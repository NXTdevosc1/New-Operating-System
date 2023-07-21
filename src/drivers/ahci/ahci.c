#include <ahci.h>

NSTATUS AhciInitDevice(PCI_DEVICE_LOCATION* ploc) {
    KDebugPrint("Found AHCI Controller at location %x", ploc);

    PAHCI Ahc = AllocateNullPool(sizeof(AHCI));
    if(!Ahc) return STATUS_OUT_OF_MEMORY;

    Ahc->Hba = PciGetBaseAddress(&Pci, ploc, 5);
    KeMapVirtualMemory(NULL, (void*)Ahc->Hba, (void*)Ahc->Hba, AHCI_CONFIGURATION_PAGES, PAGE_WRITE_ACCESS, PAGE_CACHE_DISABLE);

    KDebugPrint("AHCI Base Address %x", Ahc->Hba);

    // Enable the AHC
    Ahc->Hba->GlobalHostControl.AhciEnable = 1;

    // Fill the rest of the structure
    Ahc->Ports = (void*)(((char*)Ahc->Hba) + AHCI_PORTS_OFFSET);

    Ahc->Device = KeCreateDevice(DEVICE_CONTROLLER, 0, L"Advanced Host Controller Interface (AHCI)", Ahc);
    if(!Ahc->Device) {
        AhciAbort(Ahc, STATUS_UNSUPPORTED);
    }


    if(Ahc->Hba->ExtendedHostCapabilities.BiosOsHandoff) {
        KDebugPrint("AHC Supports BIOS/OS Handoff");
        Ahc->Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore = 1;
        while(!Ahc->Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore);
        while(Ahc->Hba->BiosOsHandoffControlAndStatus.BiosBusy);
        while(Ahc->Hba->BiosOsHandoffControlAndStatus.BiosOwnedSemaphore);
        KDebugPrint("AHCI Ownership taken");
    }

    if(Ahc->Hba->HostCapabilities.x64AddressingCapability) {
        Ahc->LongAddresses = TRUE;
    }
    AhciReset(Ahc);
    Ahc->NumSlots = Ahc->Hba->HostCapabilities.NumCommandSlots + 1;

    KDebugPrint("AHCI Enabled successfully NUM_CMD_SLOTS %d", Ahc->NumSlots);
    
    // Enable MSI Interrupts
    EnableMsiInterrupts(&Pci, ploc);
    return STATUS_SUCCESS;
}