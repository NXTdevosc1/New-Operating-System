#include <ddk.h>
#include <eodx.h>

NSTATUS NOSAPI DriverEntry(PDRIVER Driver) {
    KDebugPrint("EODX Driver Startup. DriverID %u", Driver->DriverId);
    return STATUS_SUCCESS;
}