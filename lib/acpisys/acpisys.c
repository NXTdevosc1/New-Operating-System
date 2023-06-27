#include <ddk.h>
#define ACPISYSTEM __declspec(dllexport) __fastcall
#include <AcpiSystem/acpi.h>

HANDLE AcpiHandle = INVALID_HANDLE;

NSTATUS AcpiLibEntry() {
    KDebugPrint("ACPI Library entry.");
    // You can use NULL to refer to the system process
    NSTATUS Status = ObOpenHandleByName(NULL, OBJECT_DEVICE, "Advanced Configuration And Power Interface", 0, &AcpiHandle);
    if(NERROR(Status)) return Status;
    return STATUS_SUCCESS;
}

BOOLEAN ACPISYSTEM AcpiGetVersion(UINT32* Version) {
    if(AcpiHandle == INVALID_HANDLE) return FALSE;
    return TRUE;
}