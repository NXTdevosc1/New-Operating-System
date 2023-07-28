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

    Port->OsDriveIdentify.SectorSize = 0x200;
    if(Port->AtaDeviceIdentify->PhysicalLogicalSectorSize.MultipleLogicalSectorsPerPhysicalSector) {
        if(Port->AtaDeviceIdentify->PhysicalLogicalSectorSize.LogicalSectorLongerThan256Words) {
            Port->OsDriveIdentify.SectorSize = Port->AtaDeviceIdentify->WordsPerLogicalSector << 1;
            KDebugPrint("Logical sector longer than 512 bytes, LS = %u bytes", Port->OsDriveIdentify.SectorSize);
        }

        Port->OsDriveIdentify.SectorSize *= (Port->AtaDeviceIdentify->PhysicalLogicalSectorSize.LogicalSectorsPerPhysicalSector + 1);

        KDebugPrint("Multiple logical sectors per physical sector, Sector size : %u", Port->OsDriveIdentify.SectorSize);
    }

    Port->OsDriveIdentify.NumSectors = Port->AtaDeviceIdentify->CurrentSectorCapacity;
    if(Port->AtaDeviceIdentify->CommandSetActive.BigLba) {
        Port->OsDriveIdentify.NumSectors = Port->AtaDeviceIdentify->Max48BitLBA + 1;
    }

    KDebugPrint("Model Number : %ls Sector size : %u Bytes, Disk Size : %u MB", Port->OsDriveIdentify.Name,
    Port->OsDriveIdentify.SectorSize, (Port->OsDriveIdentify.NumSectors * Port->OsDriveIdentify.SectorSize) / 0x100000
    );

    UINT NumBytes = 0x100000;

    char* Buff = AhciAllocate(Port->Ahci, NumBytes >> 12, 0);

    KDebugPrint("Testing read...");
    NSTATUS Status = AhciSataRead(Port, 1, NumBytes >> 9, Buff);

    KDebugPrint("Read status %x Buffer: %x", Status, *(UINT64*)Buff);
    KDebugPrint(Buff);

    Port->OsDriveIdentify.Device = KeCreateDevice(DEVICE_DISK, 0, Port->OsDriveIdentify.Name, Port);
    if(!Port->OsDriveIdentify.Device) {
        KDebugPrint("Failed to create AHCI Device");
        return;
    }

}


NSTATUS AhciSataRead(PAHCIPORT Port, UINT64 Lba, UINT64 NumSectors, char* Buffer) {
    if((Lba + NumSectors) >= Port->OsDriveIdentify.NumSectors) return STATUS_OUT_OF_BOUNDS;
    if(((UINT64)Buffer & 0xF)) return STATUS_INVALID_PARAMETER;

    UINT64 Operations = 0;
    UINT64 Done = 0;
    for(;NumSectors;Operations++) {
        UINT16 Chunk = NumSectors & 0xFFFF;

        UINT32 CommandSlot = AhciAllocateSlot(Port);
        AHCI_COMMAND_LIST_ENTRY* Entry = Port->CommandList + CommandSlot;
        AHCI_COMMAND_TABLE* Cmd = Port->CommandTable + CommandSlot;

        Entry->CommandFisLength = sizeof(ATA_FIS_H2D) >> 2;
        Entry->PrdtByteCount = 0;
        Entry->PrdtLength = 1;
        ATA_FIS_H2D* Fis = (ATA_FIS_H2D*)Cmd->CommandFis;
        Fis->FisType = FIS_TYPE_H2D;
        Fis->CommandControl = 1;
        Fis->Command = ATA_READ_DMA_EX;
        Fis->Count = Chunk;
        Fis->Device = AHCI_DEVICE_LBA;
        Entry->Write = 0;

        Fis->Lba0 = Lba;
        Fis->Lba1 = Lba >> 16;
        Fis->Lba2 = Lba >> 24;
        Fis->Lba3 = Lba >> 40;

        Cmd->Prdt->DataBaseAddress = AhciPhysicalAddress(Buffer);
        Cmd->Prdt->DataByteCount = (Chunk * Port->OsDriveIdentify.SectorSize) - 1;

        AhciIssueCommandAsync(Port, CommandSlot, &Done);

        Buffer+=(Chunk * Port->OsDriveIdentify.SectorSize);
        NumSectors-=Chunk;
    }

    while(Done != Operations) Sleep(1);

    return STATUS_SUCCESS;
}

NSTATUS AhciSataWrite(PAHCIPORT Port, UINT64 Lba, UINT64 NumSectors, char* Buffer) {
    if((Lba + NumSectors) >= Port->OsDriveIdentify.NumSectors) return STATUS_OUT_OF_BOUNDS;
    if(((UINT64)Buffer & 0xF)) return STATUS_INVALID_PARAMETER;

    UINT64 Operations = 0;
    UINT64 Done = 0;
    for(;NumSectors;Operations++) {
        UINT16 Chunk = NumSectors & 0xFFFF;

        UINT32 CommandSlot = AhciAllocateSlot(Port);
        AHCI_COMMAND_LIST_ENTRY* Entry = Port->CommandList + CommandSlot;
        AHCI_COMMAND_TABLE* Cmd = Port->CommandTable + CommandSlot;

        Entry->CommandFisLength = sizeof(ATA_FIS_H2D) >> 2;
        Entry->PrdtByteCount = 0;
        Entry->PrdtLength = 1;
        ATA_FIS_H2D* Fis = (ATA_FIS_H2D*)Cmd->CommandFis;
        Fis->FisType = FIS_TYPE_H2D;
        Fis->CommandControl = 1;
        Fis->Command = ATA_WRITE_DMA_EX;
        Fis->Count = Chunk;
        Fis->Device = AHCI_DEVICE_LBA;
        Entry->Write = 1;

        Fis->Lba0 = Lba;
        Fis->Lba1 = Lba >> 16;
        Fis->Lba2 = Lba >> 24;
        Fis->Lba3 = Lba >> 40;

        Cmd->Prdt->DataBaseAddress = AhciPhysicalAddress(Buffer);
        Cmd->Prdt->DataByteCount = (Chunk * Port->OsDriveIdentify.SectorSize) - 1;

        AhciIssueCommandAsync(Port, CommandSlot, &Done);

        Buffer+=(Chunk * Port->OsDriveIdentify.SectorSize);
        NumSectors-=Chunk;
    }

    while(Done != Operations) Sleep(1);

    return STATUS_SUCCESS;
}