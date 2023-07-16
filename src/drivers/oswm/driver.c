#include <ddk.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("Window Management Driver startup");
    return STATUS_SUCCESS;
}
