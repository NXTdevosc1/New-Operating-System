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
        UINT8 DevClass = Pci.Read8(&Context->DeviceData.PciDeviceData.PciDeviceLocation, PCI_CLASS);
        UINT8 DevSubclass = Pci.Read8(&Context->DeviceData.PciDeviceData.PciDeviceLocation, PCI_SUBCLASS);
        UINT8 ProgIf = Pci.Read8(&Context->DeviceData.PciDeviceData.PciDeviceLocation, PCI_PROGIF);
       
        if(DevClass == 1 && DevSubclass == 6 && ProgIf == 1) {
            KDebugPrint("Found AHCI Controller at addresscode %x", Context->DeviceData.PciDeviceData.PciDeviceLocation.Address);
            PCI_DEVICE_LOCATION* DeviceConfig = &Context->DeviceData.PciDeviceData.PciDeviceLocation;
            HBA_REGISTERS* Hba = PciGetBaseAddress(&Pci, DeviceConfig, 5);
            KDebugPrint("HBA %x", Hba);

            KeMapVirtualMemory(NULL, Hba, Hba, AHCI_CONFIGURATION_PAGES, PAGE_WRITE_ACCESS, PAGE_CACHE_DISABLE);

            Hba->GlobalHostControl.AhciEnable = 1;
            
        }
    }
    return STATUS_SUCCESS;
}