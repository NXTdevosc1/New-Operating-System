#include <nos/nos.h>
#include <nos/sys/sys.h>
#include <crt.h>

PVOID KRNLAPI KeFindSystemFirmwareTable(
    IN char* FirmwareTableId,
    IN OPT GUID* EfiFirmwareTableGuid
) {
    UINT len = strlen(FirmwareTableId);
    if(len >= 0x200) return NULL;
    EFI_CONFIGURATION_TABLE* FirmwareTable = NosInitData->EfiConfigurationTable;
    for(UINT64 i = 0;i<NosInitData->NumConfigurationTableEntries;i++, FirmwareTable++) {
        if(EfiFirmwareTableGuid) {
            if(FirmwareTable->VendorGuid.Data1 != EfiFirmwareTableGuid->Data1 ||
            FirmwareTable->VendorGuid.Data2 != EfiFirmwareTableGuid->Data2 ||
            FirmwareTable->VendorGuid.Data3 != EfiFirmwareTableGuid->Data3 ||
            *(UINT64*)FirmwareTable->VendorGuid.Data4 != *(UINT64*)EfiFirmwareTableGuid->Data4
            ) continue;
        }
        if(!KeCheckMemoryAccess(NULL, FirmwareTable->VendorTable, len, NULL)) {
            KeMapVirtualMemory(
                NULL,
                FirmwareTable->VendorTable,
                FirmwareTable->VendorTable,
                1,
                PAGE_WRITE_ACCESS,
                0
            );
        }
        if(memcmp(FirmwareTableId, FirmwareTable->VendorTable, len) == 0) {
            return FirmwareTable->VendorTable;
        }
    }
    return NULL;
}