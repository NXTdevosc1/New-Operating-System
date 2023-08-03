#include <fat32.h>


// Returns next cluster
UINT32 FatReadCluster(FAT32_INSTANCE* fs, UINT Cluster, void* Buffer) {
    if(Cluster < 2) {
        KDebugPrint("FAT32 Error : Read Cluster below 2");
        while(1) __halt();
    }

    UINT64 Lba = fs->FatLba + (Cluster >> 7);
    if(Lba >= fs->FatLba + fs->BootSector->ExtendedBiosParamBlock.SectorsPerFat) {
        KDebugPrint("FAT32 Error: Read FAT Exceeds FAT Size");
        while(1) __halt();
    }

    if(fs->LastReadFatLba != Lba) {
        if(!KeReadDrive(fs->Drive, Lba, 1, fs->LastReadFatBuffer)) {
            KDebugPrint("FAT32 Error : FAT_READ_CLUSTER Failed to read drive.");
            while(1) __halt();
        }
        fs->LastReadFatLba = Lba;
    }

    KeReadDrive(fs->Drive,
    fs->RootDirLba + (Cluster - 2) * fs->ClusterSize,
    fs->ClusterSize,
    Buffer
    );

    return fs->LastReadFatBuffer[Cluster & 0x7F];
}