#include "sddkinternal.h"
#include <pcidef.h>

BOOLEAN SYSAPI PciQueryInterface(OUT PCI_DRIVER_INTERFACE* DriverInterface) {
    HANDLE PciHandle = INVALID_HANDLE;
    // Search for the PCI Device
    UINT64 _ev = 0;
    PDEVICE Device;
    if(KeEnumerateDevices(NULL, &Device, L"Peripheral Component Interconnect", FALSE, &_ev)) {
        if(NERROR(ObOpenHandle(KeGetDeviceObjectHeader(Device), NULL, 0, &PciHandle)) &&
        PciHandle == INVALID_HANDLE // Probably the handle is already open
        ) {
            KDebugPrint("Failed to open PCI Handle");
            return FALSE;
        }
        BOOLEAN Ret = (BOOLEAN)IoProtocol(PciHandle, 0, 1, &DriverInterface);
        ObCloseHandle(NULL, PciHandle);
        return Ret;
    }
    KDebugPrint("No PCI was found");
    return FALSE;
}

NSTATUS SYSAPI EnableMsiInterrupts(PCI_DRIVER_INTERFACE* Pci, PCI_DEVICE_LOCATION* Location) {
    if(!(Pci->Read8(Location, PCI_STATUS) & (1 << 4))) return STATUS_UNSUPPORTED; // Capabilites are not supported
    KDebugPrint("Enabling MSI Interrupts Device %d Bus %d Function %d", Location->Fields.Device, Location->Fields.Bus, Location->Fields.Bus);
    UINT8 Cptr = Pci->Read8(Location, PCI_CAPABILITYPTR) & ~3;
    while(Cptr) {
        UINT8 CapId = Pci->Read8(Location, Cptr + PCI_CAPABILITY_ID);
        KDebugPrint("cptr %x capid %x", Cptr, CapId);
        if(CapId == PCI_MSI_CAPABILITY) {
            KDebugPrint("Found MSI Capability");
        }
        Cptr = Pci->Read8(Location, Cptr + PCI_CAPABILITY_NEXT) & ~3;
    }
    return STATUS_UNSUPPORTED;
}