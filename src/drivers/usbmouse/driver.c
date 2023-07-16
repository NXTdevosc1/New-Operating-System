#include <ddk.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("USB Mouse Driver startup");
    return STATUS_SUCCESS;
}
