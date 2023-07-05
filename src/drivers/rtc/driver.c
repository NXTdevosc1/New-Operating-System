#include <ddk.h>
#include <acpisystem/acpi.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    UINT32 Ver;
    KDebugPrint("RTC Driver startup. %x", Ver);
    if(!AcpiGetVersion(&Ver)) {
        KDebugPrint("Failed to get acpi version.");
        return STATUS_SUBSYSTEM_NOT_PRESENT;
    }
    KDebugPrint("ACPI Version : %u.%u", Ver >> 16, Ver & 0xFFFF);
    return STATUS_SUCCESS;
}