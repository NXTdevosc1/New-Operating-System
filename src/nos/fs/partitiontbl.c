#include <nos/nos.h>
#include <nos/fs/fs.h>
#include <nos/fs/mbr.h>
#include <nos/fs/gpt.h>
#include <kevent.h>
#include <kfs.h>

void KiReadGuidPartitionTable(PDRIVE Drive, MBR_PARTITION_RECORD* Record);

static GUID NullGuid = {0};
static GUID BasicDataPartitionGuid = BASIC_DATA_PARTITION_GUID;
static GUID EfiSystemPartitionGuid = EFI_SYSTEM_PARTITION_GUID;

HANDLE PartitionAddEvent = INVALID_HANDLE;

NSTATUS KiReadPartitionTable(PDRIVE Drive) {

    if(PartitionAddEvent == INVALID_HANDLE) {
        PartitionAddEvent = KeOpenEvent(SYSTEM_EVENT_PARTITION_ADD);
        if(PartitionAddEvent == INVALID_HANDLE) KeRaiseException(STATUS_EVENT_OPEN_FAILED);
    }

    KDebugPrint("Reading partition table for drive : %ls", Drive->DriveId->Name);
    
    PVOID Buffer = KeAllocateDriveBuffer(Drive, 1);
    if(!Buffer) return STATUS_OUT_OF_MEMORY;

    // Read first sector (MBR)

    MASTER_BOOT_RECORD* Mbr = Buffer;
    

    KeReadDrive(Drive, 0, 1, Buffer);

    if(Mbr->Signature != MBR_SIGNATURE) {
        KeFreeDriveBuffer(Drive, Buffer, 1);
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
    KeFreeDriveBuffer(Drive, Buffer, 1);
    return STATUS_SUCCESS;
}

BOOLEAN CmpGuid(GUID* Guid1, GUID* Guid2) {
    return (memcmp(Guid1, Guid2, 0x10) == 0);
}

void KiReadGuidPartitionTable(PDRIVE Drive, MBR_PARTITION_RECORD* Record) {
    PARTITION_TABLE_HEADER* Gpt = KeAllocateDriveBuffer(Drive, 1);
    if(!Gpt) return;

    KeReadDrive(Drive, Record->StartingLBA, 1, Gpt);

    if(Gpt->Signature != GPT_SIGNATURE) {
        KDebugPrint("Invalid GPT Header in drive#%u", Drive->DriveId);
        return;
    }

    UINT NumSects = (Gpt->NumPartitions * Gpt->PartitionEntrySize) / Drive->DriveId->SectorSize + 1;

    GUID_PARTITION_ENTRY* AllPartitions = KeAllocateDriveBuffer(Drive, NumSects);
    if(!AllPartitions) {
        KDebugPrint("READ_GPT BUG0");
        while(1) __halt();
    }

    KDebugPrint("GPT Signature %s EntrySize %x PartitionsLba %x NumEntries %x", &Gpt->Signature, Gpt->PartitionEntrySize, Gpt->PartitionsLba, (UINT64)Gpt->NumPartitions);
    KeReadDrive(Drive, Gpt->PartitionsLba, NumSects, AllPartitions);

    for(UINT i = 0;i<Gpt->NumPartitions;i++) {
        GUID_PARTITION_ENTRY* Partition = (GUID_PARTITION_ENTRY*)((char*)AllPartitions + i * Gpt->PartitionEntrySize);
        if(CmpGuid(&NullGuid, &Partition->PartitionTypeGuid)) continue;
        KDebugPrint("Partition#%u FIRST_LBA %x LAST_LBA %x Name : %ls", i, Partition->FirstLba, Partition->LastLba, Partition->PartitionName);
        
        SYSTEM_PARTITION_ADD_CONTEXT* Context = MmAllocatePool(sizeof(SYSTEM_PARTITION_ADD_CONTEXT), 0);
        Context->Drive = Drive;
        Context->StartLba = Partition->FirstLba;
        Context->EndLba = Partition->LastLba;
        Context->GuidPartition = TRUE;
        memcpy(&Context->PartitionInstanceGuid, &Partition->UniquePartitionGuid, 0x10);
        Context->PartitionAttributes = 0;


        if(CmpGuid(&BasicDataPartitionGuid, &Partition->PartitionTypeGuid)) {
            KDebugPrint("Basic data partition");
        } else if(CmpGuid(&EfiSystemPartitionGuid, &Partition->PartitionTypeGuid)) {
            KDebugPrint("EFI System partition");
        }
        KeSignalEvent(PartitionAddEvent, Context, &Context->EventDesc);
    }


    

    KeFreeDriveBuffer(Drive, AllPartitions, NumSects);
    KeFreeDriveBuffer(Drive, Gpt, 1);
}