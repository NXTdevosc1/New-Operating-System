#include <ddk.h>
#include <pcidef.h>

NSTATUS DeviceAddHandler(SYSTEM_DEVICE_ADD_CONTEXT* DeviceAddContext) {
    KDebugPrint("EHCI Device add handler");
    return STATUS_SUCCESS;
}

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("EHCI Driver startup");
    KeRegisterEventHandler(SYSTEM_EVENT_DEVICE_ADD, 0, DeviceAddHandler);
    return STATUS_SUCCESS;
}
