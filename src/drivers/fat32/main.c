#include <fat32.h>


NSTATUS NOSENTRY DriverEntry(
    PDRIVER Driver
) {
    KDebugPrint("FAT32 Driver startup, driverid#%u", Driver->DriverId);
    return (KeRegisterEventHandler(
        SYSTEM_EVENT_PARTITION_ADD, 0, Fat32PartitionAddEvt
    )) ? STATUS_SUCCESS : STATUS_UNSUPPORTED;
}

NSTATUS Fat32PartitionAddEvt(SYSTEM_PARTITION_ADD_CONTEXT* Context) {
    KDebugPrint("FAT32 : Partition add event, START %x END %x", Context->StartLba, Context->EndLba);
    
    PDRIVE drv = Context->Drive;

    FAT32_BOOTSECTOR* Bootsector = KeAllocateDriveBuffer(drv, 1);
    KeReadDrive(drv, Context->StartLba, 400, Bootsector);
    KDebugPrint("Volume %11s FS : %8s", Bootsector->ExtendedBiosParamBlock.VolumeLabel, Bootsector->ExtendedBiosParamBlock.FsLabel);
    if(Bootsector->BootSignature != 0xAA55 || Bootsector->ExtendedBiosParamBlock.ExtendedBootSignature != 0x29) {
        // This isn't a FAT32 Partition
        KeFreeDriveBuffer(drv, Bootsector, 1);
        return STATUS_INVALID_FORMAT;
    }
    
    FAT32_INSTANCE* fs = MmAllocatePool(sizeof(FAT32_INSTANCE), 0);
    if(!fs) KeRaiseException(STATUS_OUT_OF_MEMORY);
    ObjZeroMemory(fs);
    // Todo : use optimized memory transfers

    fs->Drive = drv;
    fs->ClusterSize = Bootsector->BiosParamBlock.ClusterSize;
    fs->BootSector = Bootsector;
    fs->LastReadFatBuffer = KeAllocateDriveBuffer(drv, 1);

    KDebugPrint("Found FAT32 Partition, Cluster size %x RSV AREA %x ROOT DIR %x FAT Size %x",
    fs->BootSector->BiosParamBlock.ClusterSize,
    fs->BootSector->BiosParamBlock.ReservedAreaSize,
    fs->BootSector->ExtendedBiosParamBlock.RootDirStart,
    fs->BootSector->ExtendedBiosParamBlock.SectorsPerFat);

    fs->NumFats = fs->BootSector->BiosParamBlock.FatCount;
    fs->FatLba = Context->StartLba + fs->BootSector->BiosParamBlock.ReservedAreaSize;
    fs->RootDirLba = fs->FatLba + (fs->NumFats * fs->BootSector->ExtendedBiosParamBlock.SectorsPerFat);

    KDebugPrint("FAT32 : Num allocation tables %u FAT_LBA %u ROOTDIR_LBA %u",
    fs->NumFats,
    fs->FatLba,
    fs->RootDirLba
    );

    FatReadDirectory(fs, fs->BootSector->ExtendedBiosParamBlock.RootDirStart);

    return STATUS_SUCCESS;

}