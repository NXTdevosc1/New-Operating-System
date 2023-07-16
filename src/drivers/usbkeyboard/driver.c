#include <ddk.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("USB Keyboard Driver startup");
    return STATUS_SUCCESS;
}
