#include <nos/nos.h>
#include <nos/pnp/pnp.h>

NSTATUS DeviceEvtHandler(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access) {
    return STATUS_SUCCESS;
}
PDEVICE KRNLAPI PnpCreateDevice(
    UINT DeviceType,
    UINT64 DeviceCharacteristics,
    UINT16* DevicePath, 
    UINT16* DisplayName
) {
    PDEVICE Device = MmAllocatePool(sizeof(DEVICE), 0);

    Device->DeviceType = DeviceType;
    Device->DeviceCharacteristics = DeviceCharacteristics;

    Device->DevicePathLength = wcslen(DevicePath);
    Device->DisplayNameLength = wcslen(DisplayName);

    Device->DevicePath = KiMakeSystemNameW(DevicePath, Device->DevicePathLength);
    Device->DisplayName = KiMakeSystemNameW(DisplayName, Device->DisplayNameLength);


    NSTATUS S = ObCreateObject(&Device->ObjectDescriptor,
    OBJECT_PERMANENT,
    OBJECT_DEVICE,
    "DEV0",
    Device,
    DeviceEvtHandler
    );
    if(NERROR(S)) {
        MmFreePool(Device->DevicePath);
        MmFreePool(Device->DisplayName);
        MmFreePool(Device);

        return NULL;
    }

    return Device;
}

BOOLEAN KRNLAPI PnpDeleteDevice(PDEVICE Device) {
    HANDLE h;
    if(NERROR(ObOpenHandleByAddress(KernelProcess, Device, OBJECT_DEVICE, 0, &h))) return FALSE;

    ObCloseHandle(KernelProcess, h);

    // Do not force because operations may be still done on the device
    if(!ObDestroyObject(Device->ObjectDescriptor, 0)) return FALSE;

    MmFreePool(Device->DevicePath);
    MmFreePool(Device->DisplayName);
    MmFreePool(Device);

    return TRUE;

}