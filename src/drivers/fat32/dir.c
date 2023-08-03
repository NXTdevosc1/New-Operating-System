#include <fat32.h>

static inline UINT ReadLfn(FAT32_LONG_FILE_NAME_ENTRY* Lfe, UINT16* Out) {
    UINT Len = 0;
    for(int c = 0;c<5;c++) {
        if(Lfe->Name0[c] == 0xFFFF || Lfe->Name0[c] == 0) break;
        *Out = Lfe->Name0[c];
        Out++;
        Len++;
    }
    for(int c = 0;c<6;c++) {
        if(Lfe->Name1[c] == 0xFFFF || Lfe->Name1[c] == 0) break;
        *Out = Lfe->Name1[c];
        Out++;
        Len++;
    }
    for(int c = 0;c<2;c++) {
        if(Lfe->Name2[c] == 0xFFFF || Lfe->Name2[c] == 0) break;
        *Out = Lfe->Name2[c];
        Out++;
        Len++;
    }
    *Out = 0;

    return Len;
}

static inline void ReadSfn(FAT32_FILE_ENTRY* Sfe, UINT16* Out) {
     // Short file name
    UINT8 LastChar = 0;
    for(int c =0;c<8;c++) {
        Out[c] = Sfe->ShortFileName[c];
        if(Sfe->ShortFileName[c] > 0x20) LastChar = c + 1;
    }

    // Short file extension
    for(int c = 0;c<3;c++) {
        if(Sfe->ShortFileExtension[c] <= 0x20) break;
        if(!c) {
            Out[LastChar] = L'.';
            LastChar++;
        }
        Out[LastChar] = Sfe->ShortFileExtension[c];
        LastChar++;
    }
    Out[LastChar] = 0;
}

BOOLEAN FatReadDirectory(
    FAT32_INSTANCE* fs,
    UINT Cluster
) {
    FAT32_FILE_ENTRY* File = FatAllocateClusters(fs, 1);
    // Make sure that sector size aligns with file entry size
    void* Start = File;
    char* end = (char*)AlignBackward((char*)File + (fs->ClusterSize * fs->Drive->DriveId->SectorSize), sizeof(FAT32_FILE_ENTRY));

    UINT16 FileName[0x100] = {0};
    UINT16 StrLfn[0x20];
    UINT16* NameProgress;
    BOOLEAN Lfn = FALSE;
    while(Cluster < FAT32_ENDOF_CHAIN) {
        Cluster = FatReadCluster(fs, Cluster, File);

        for(File = Start;(char*)File < (char*)end;File++) {
            if(File->ShortFileName[0] == 0) goto ReadEnd;
            if(File->ShortFileName[0] == 0xE5) continue; // Deleted entry
            if((File->FileAttributes & 0xF) == 0xF) {
                // Long file name start
                UINT Len = ReadLfn((void*)File, StrLfn);
                if(!Lfn) {
                    Lfn = TRUE;
                    NameProgress = FileName + 0xFF - Len;
                } else {
                    NameProgress -= Len;
                }
                // Also copy trailing end character \0
                memcpy(NameProgress, StrLfn, Len << 1);
            } else {
                UINT16* Name;
                if(Lfn) {
                    Name = NameProgress;
                    Lfn = FALSE;
                } else {
                    ReadSfn(File, FileName);
                    Name = FileName;
                }

                KDebugPrint("File(%x, %u Bytes) : %ls Last Modified %u/%u/%u at %u:%u:%u", File->FileAttributes, File->FileSize, Name,
                File->LastModifiedDate.Day, File->LastModifiedDate.Month, File->LastModifiedDate.YearFrom1980 + 1980,
                File->LastModifiedTime.Hours, File->LastModifiedTime.Minutes, File->LastModifiedTime.SecondsDivBy2 * 2
                );
            }
        } 
    }
ReadEnd:
    KeFreeDriveBuffer(fs->Drive, File, fs->ClusterSize);

    return TRUE;
}