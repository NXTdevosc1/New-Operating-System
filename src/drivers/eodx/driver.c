#include <ddk.h>
#include <eodx.h>

NSTATUS DriverEntry(void* Driver) {
    KDebugPrint("EODX Driver Startup.");
    return STATUS_SUCCESS;
}