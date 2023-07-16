#include <ddk.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("AHCI Driver startup");
    return STATUS_SUCCESS;
}
