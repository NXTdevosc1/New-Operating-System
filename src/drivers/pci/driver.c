#include <ddk.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("PCI Driver startup");
    return STATUS_SUCCESS;
}
