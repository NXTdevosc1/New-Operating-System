#include <ddk.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("USB Flash Drive Driver startup");
    return STATUS_SUCCESS;
}
