#include <ahci.h>

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("AHCI Driver startup");
    KeRegisterEventHandler(SYSTEM_EVENT_PCI_DEVICE_ADD, 0, (SYSTEM_EVENT_HANDLER)PciAddEvent);
    return STATUS_SUCCESS;
}

NSTATUS PciAddEvent(PCI_DEVICE_ADD_CONTEXT* Context) {
    KDebugPrint("AHCI PCI Device ADD Event");
    return STATUS_SUCCESS;
}