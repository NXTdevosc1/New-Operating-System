#include <ahci.h>

void AhciDisablePort(PAHCIPORT Port) {
    Port->HbaPort->CommandStatus &= ~PORTxCMDxSTART;
    while(Port->HbaPort->CommandStatus & PORTxCMDxCR) _mm_pause();
    Port->HbaPort->CommandStatus &= ~PORTxCMDxFRE;
    while(Port->HbaPort->CommandStatus & PORTxCMDxFRR) _mm_pause();
}

void AhciEnablePort(PAHCIPORT Port) {
    Port->HbaPort->CommandStatus |= (PORTxCMDxSTART | PORTxCMDxFRE);
    while((Port->HbaPort->CommandStatus & (PORTxCMDxCR | PORTxCMDxFRR)) != (PORTxCMDxCR | PORTxCMDxFRR)) _mm_pause();
}

void AhciInitPort(PAHCIPORT Port) {

    KDebugPrint("AHCI Thread#%u : Initializing port %u", KeGetCurrentThreadId(), Port->PortIndex);

    Port->AllocatedCmd = ~(((UINT32)-1) >> (31 - Port->Ahci->MaxSlotNumber));

    KDebugPrint("AHCI Thread%u Allocating resources", KeGetCurrentThreadId());

    Port->CommandList = AhciAllocate(Port->Ahci, ConvertToPages(sizeof(AHCI_COMMAND_LIST_ENTRY) * (Port->Ahci->MaxSlotNumber + 1)), 0);
    Port->CommandTable = AhciAllocate(Port->Ahci, ConvertToPages(sizeof(AHCI_COMMAND_TABLE) * (Port->Ahci->MaxSlotNumber + 1)), 0);
    Port->ReceivedFis = AhciAllocate(Port->Ahci, 1, 0);

    if(!Port->CommandList || !Port->CommandTable || !Port->ReceivedFis) {
        KDebugPrint("AHCI Thread#%u Failed to allocate resources", KeGetCurrentThreadId());
        return;
    }

    KDebugPrint("AHCI Thread%u disabling port", KeGetCurrentThreadId());

    // Stop the port
    AhciDisablePort(Port);


    KDebugPrint("AHCI Thread%u port disabled successfully, setting bars", KeGetCurrentThreadId());

    UINT64 rf = AhciPhysicalAddress(Port->ReceivedFis), cl = AhciPhysicalAddress(Port->CommandList), ct = AhciPhysicalAddress(Port->CommandTable);

    Port->_PhysReceivedFis = (void*)rf;
    Port->_PhysCommandList = (void*)cl;
    Port->_PhysCommandTable = (void*)ct;

    Port->HbaPort->FisBaseAddressLow = rf;
    Port->HbaPort->FisBaseAddressHigh = rf >> 32;

    Port->HbaPort->CommandListBaseAddressLow = cl;
    Port->HbaPort->CommandListBaseAddressHigh = cl >> 32;

    KDebugPrint("AHCI Thread #%u Init command list", KeGetCurrentThreadId());
    // Init command list
    for(int i = 0;i<=Port->Ahci->MaxSlotNumber;i++) {
        Port->CommandList[i].PrdtLength = 1;
        Port->CommandList[i].CommandTableAddress = (UINT64)(Port->_PhysCommandTable + i);
    }

    *(volatile UINT32*)&Port->HbaPort->SataError = -1;
    
    // Enable all interrupts
    Port->HbaPort->InterruptEnable = -1;

    Port->HbaPort->CommandStatus |= PORTxCMDxFRE;
    while(!(Port->HbaPort->CommandStatus & PORTxCMDxFRR));

    // Enable all interrupts (Another time)
    Port->HbaPort->InterruptEnable = -1;

    BOOLEAN DeviceDetected = FALSE;

    if(Port->Ahci->Hba->HostCapabilities.SupportsStaggeredSpinup) {
        KDebugPrint("Port Staggered Spinup");
        Port->HbaPort->CommandStatus |= PORTxCMDxSUD;
        Sleep(100);
    }
    if(Port->HbaPort->CommandStatus & PORTxCMDxCPD) {
        Port->HbaPort->CommandStatus |= PORTxCMDxPOD;
    }
    if(Port->Ahci->Hba->HostCapabilities.SlumberStateCapable) {
        UINT32 v = Port->HbaPort->CommandStatus;
        v &= ~((0x1F) << PORTxCMDxICC);
        v |= 1 << PORTxCMDxICC;
        Port->HbaPort->CommandStatus = v;
    }
    *(volatile UINT32*)&Port->HbaPort->SataControl = 0x301;
    Sleep(5);
    *(volatile UINT32*)&Port->HbaPort->SataControl = 0x300;

    UINT Countdown = 20;
    for(;;) {
        if(!Countdown) break;
        Countdown--;
        if(Port->HbaPort->SataStatus.DeviceDetection == 3) {
            KDebugPrint("Device detected on port %d", Port->PortIndex);
            DeviceDetected = 1;
            break;
        }
        Sleep(1);
    }

    // Wait for the first D2H FIS Containing device signature and identification

    Countdown = 50;
    while(Port->HbaPort->TaskFileData.Busy || Port->HbaPort->TaskFileData.DataTransferRequested) {
        if(!Countdown) {
            DeviceDetected = 0;
            break;
        }   
        Countdown--;
        Sleep(1);
    }

    // Wait for the interrupt
    Countdown = 25;
    while(!Port->FirstD2H) {
        if(!Countdown) {
            DeviceDetected = 0;
            break;
        }
        Countdown--;
        Sleep(1);
    }

    *(volatile UINT32*)&Port->HbaPort->SataError = -1;


    if(!DeviceDetected) {
        KDebugPrint("No device detected on port %d", Port->PortIndex);
        if(Port->Ahci->Hba->HostCapabilities.SupportsStaggeredSpinup) {
            Port->HbaPort->CommandStatus &= ~PORTxCMDxSUD;
        }
        return;
    }

    KDebugPrint("Port signature %x", Port->HbaPort->PortSignature.Signature);

    if(Port->HbaPort->PortSignature.Signature == 0xEB140101) {
        Port->Atapi = TRUE;
        Port->HbaPort->CommandStatus |= PORTxCMDxATAPI;
        AHCI_COMMAND_LIST_ENTRY* e = Port->CommandList;
        for(UINT i = 0;i<=Port->Ahci->MaxSlotNumber;i++) {
            e->Atapi = 1;
        }
    }



    AhciEnablePort(Port);


    if(Port->Atapi == FALSE) {
        AhciInitSataDevice(Port);
    } else {

    }
}