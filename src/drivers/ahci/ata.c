#include <ahci.h>

void AhciInitAtaDevice(PAHCIPORT Port) {

    Port->AtaDeviceIdentify = AhciAllocate(Port->Ahci, ConvertToPages(sizeof(ATA_IDENTIFY_DEVICE_DATA)), 0);

    UINT32 Cmd = AhciAllocateSlot(Port);
    KDebugPrint("AHCI : Initializing ATA Device on Port #%u IDENTIFY_CMD#%u", Port->PortIndex, Cmd);
    AHCI_COMMAND_LIST_ENTRY* Entry = Port->CommandList + Cmd;
    Entry->CommandFisLength = sizeof(ATA_FIS_H2D) >> 2;
    ATA_FIS_H2D* IdentifyFis = (ATA_FIS_H2D*)Port->CommandTable[Cmd].CommandFis;
    IdentifyFis->Command = ATA_IDENTIFY_DEVICE;
    IdentifyFis->Device = AHCI_DEVICE_HOST;
    IdentifyFis->FisType = FIS_TYPE_H2D;
    IdentifyFis->Count = 0;
    IdentifyFis->CommandControl = 1;
    Port->CommandTable[Cmd].Prdt->DataBaseAddress = AhciPhysicalAddress(Port->AtaDeviceIdentify);
    Port->CommandTable[Cmd].Prdt->DataByteCount = sizeof(ATA_IDENTIFY_DEVICE_DATA) - 1;

    Entry->PrdtLength = 1;

    AhciIssueCommandSync(Port, Cmd);
    AhciReadModelNumber(Port);
    KDebugPrint("Model Number : %ls", Port->OsDriveIdentify.Name);
}