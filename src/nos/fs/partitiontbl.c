#include <nos/nos.h>
#include <nos/fs/fs.h>
#include <nos/fs/mbr.h>
#include <nos/fs/gpt.h>


void KiReadGuidPartitionTable(PDRIVE Drive, MBR_PARTITION_RECORD* Record);

static GUID NullGuid = {0};
static GUID BasicDataPartitionGuid = BASIC_DATA_PARTITION_GUID;
static GUID EfiSystemPartitionGuid = EFI_SYSTEM_PARTITION_GUID;

NSTATUS KiReadPartitionTable(PDRIVE Drive) {
    KDebugPrint("Reading partition table for drive : %ls", Drive->DriveId->Name);
    
    PVOID Buffer = Drive->Io.Allocate(Drive->Context, 1);
    if(!Buffer) return STATUS_OUT_OF_MEMORY;

    // Read first sector (MBR)

    MASTER_BOOT_RECORD* Mbr = Buffer;
    

    Drive->Io.Read(Drive->Context, 0, 1, Buffer);

    if(Mbr->Signature != MBR_SIGNATURE) {
        Drive->Io.Free(Drive->Context, Buffer, 1);
        return STATUS_INVALID_FORMAT;
    }

    KDebugPrint("Buffer %x %s", Buffer);
    KDebugPrint("MBR Sig %x Unique Sig %x %x %x %x", Mbr->Signature, Mbr->UniqueMbrSignature[0], Mbr->UniqueMbrSignature[1], Mbr->UniqueMbrSignature[2], Mbr->UniqueMbrSignature[3]);
    for(int i = 0;i<4;i++) {
        if(!Mbr->Partition[i].OSIndicator) continue;
        KDebugPrint("MBR Partition found, FSID %x", Mbr->Partition[i].OSIndicator);
        if(Mbr->Partition[i].OSIndicator == PMBR_GPT_PARTITION) {
            KDebugPrint("GPT Partition found");
            KiReadGuidPartitionTable(Drive, Mbr->Partition + i);
        }
    }
    
    KDebugPrint("%u Partitions were found.", Drive->NumPartitions);
    Drive->Io.Free(Drive->Context, Buffer, 1);
    return STATUS_SUCCESS;
}

BOOLEAN CmpGuid(GUID* Guid1, GUID* Guid2) {
    return (memcmp(Guid1, Guid2, 0x10) == 0);
}

void KiReadGuidPartitionTable(PDRIVE Drive, MBR_PARTITION_RECORD* Record) {
    PARTITION_TABLE_HEADER* Gpt = Drive->Io.Allocate(Drive->Context, 1);
    if(!Gpt) return;

    Drive->Io.Read(Drive->Context, Record->StartingLBA, 1, Gpt);

    if(Gpt->Signature != GPT_SIGNATURE) {
        KDebugPrint("Invalid GPT Header in drive#%u", Drive->DriveId);
        return;
    }

    UINT NumSectors = AlignForward(Gpt->NumPartitions * Gpt->PartitionEntrySize, Drive->DriveId->SectorSize) / Drive->DriveId->SectorSize;

    GUID_PARTITION_ENTRY* AllPartitions = Drive->Io.Allocate(Drive->Context, NumSectors);
    if(!AllPartitions) {
        KDebugPrint("READ_GPT BUG0");
        while(1) __halt();
    }

    KDebugPrint("GPT Signature %s EntrySize %x PartitionsLba %x NumEntries %x", &Gpt->Signature, Gpt->PartitionEntrySize, Gpt->PartitionsLba, (UINT64)Gpt->NumPartitions);
    Drive->Io.Read(Drive->Context, Gpt->PartitionsLba, NumSectors, AllPartitions);


    for(UINT i = 0;i<Gpt->NumPartitions;i++) {
        GUID_PARTITION_ENTRY* Partition = (GUID_PARTITION_ENTRY*)((char*)AllPartitions + i * Gpt->PartitionEntrySize);
        if(CmpGuid(&NullGuid, &Partition->PartitionTypeGuid)) continue;
        KDebugPrint("Partition#%u FIRST_LBA %x LAST_LBA %x Name : %ls", i, Partition->FirstLba, Partition->LastLba, Partition->PartitionName);
        if(CmpGuid(&BasicDataPartitionGuid, &Partition->PartitionTypeGuid)) {
            KDebugPrint("Basic data partition");
        } else if(CmpGuid(&EfiSystemPartitionGuid, &Partition->PartitionTypeGuid)) {
            KDebugPrint("EFI System partition");
        }
    }


    

    Drive->Io.Free(Drive->Context, AllPartitions, NumSectors);
    Drive->Io.Free(Drive->Context, Gpt, 1);
}