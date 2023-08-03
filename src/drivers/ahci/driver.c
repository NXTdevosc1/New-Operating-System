#include <ahci.h>
#include <pcidef.h>

PCI_DRIVER_INTERFACE Pci;


NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("AHCI Driver startup");
    if(!PciQueryInterface(&Pci)) {
        KDebugPrint("AHCI Driver : No PCI Found");
        return STATUS_UNSUPPORTED;
    }
    KeRegisterEventHandler(SYSTEM_EVENT_DEVICE_ADD, 0, (SYSTEM_EVENT_HANDLER)PciAddEvent);
    return STATUS_SUCCESS;
}

NSTATUS PciAddEvent(SYSTEM_DEVICE_ADD_CONTEXT* Context) {
    if(Context->BusType == BUS_PCI) {
        if(Context->DeviceData.PciDeviceData.Class == 1 &&
        Context->DeviceData.PciDeviceData.Subclass == 6 &&
        Context->DeviceData.PciDeviceData.ProgIf == 1
        ) {
            NSTATUS s = AhciInitDevice(&Context->DeviceData.PciDeviceData.PciDeviceLocation);
            
        }
    }
    return STATUS_SUCCESS;
}