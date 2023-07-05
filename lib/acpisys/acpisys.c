#include <ddk.h>
#define ACPISYSTEM __declspec(dllexport) __fastcall
#include <AcpiSystem/acpi.h>
#include <pnp.h>
#include <AcpiSystem/acpiio.h>

HANDLE AcpiHandle = INVALID_HANDLE;

NSTATUS AcpiLibEntry() {
    KDebugPrint("ACPI Library entry.");
    NSTATUS Status = STATUS_SUCCESS;
    UINT64 _EnumerationValue = 0;
    POBJECT Obj;
    PDEVICE Device;
    UINT16 DeviceName[MAX_DEVICE_NAME_LENGTH + 1];
    UINT NumDevices = 0;
    while((_EnumerationValue = ObEnumerateObjects(NULL, OBJECT_DEVICE, &Obj, NULL, _EnumerationValue))) {
        Device = ObGetAddress(Obj);

        if(PnpGetDeviceName(Device, DeviceName)) {
            KDebugPrint("Device#%d Name : %ls", NumDevices, DeviceName);

            if(memcmp(DeviceName, L"Advanced Configuration And Power Interface", 42 * 2) == 0) {
                // Check if the name starts with ACPI {version}
                KDebugPrint("Found ACPI");
                if(NERROR(ObOpenHandle(Obj, NULL, 0, &AcpiHandle))) {
                    KDebugPrint("Failed to open acpi handle.");
                    return -1;
                }
                return STATUS_SUCCESS;
            }

        
        } else {
            KDebugPrint("Device#%d has no display name.", NumDevices);
        }

        NumDevices++;
    }

    // You can use NULL to refer to the system process
    return STATUS_NOT_FOUND;
}

BOOLEAN ACPISYSTEM AcpiGetVersion(UINT32* Version) {
    if(AcpiHandle == INVALID_HANDLE) return FALSE;
    return (BOOLEAN)IoProtocol(AcpiHandle, ACPI_IO_GET_VERSION, Version);
}