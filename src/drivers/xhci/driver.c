#include <ddk.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("XHCI Driver startup");
    return STATUS_SUCCESS;
}
