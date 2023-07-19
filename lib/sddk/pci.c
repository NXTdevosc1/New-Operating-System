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