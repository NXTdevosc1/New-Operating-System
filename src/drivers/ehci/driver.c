#include <ddk.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("EHCI Driver startup");
    return STATUS_SUCCESS;
}
