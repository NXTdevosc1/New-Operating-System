#include <nos/nos.h>
#include <nos/pnp/pnp.h>

NSTATUS DeviceEvtHandler(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access) {
    if(Event == OBJECT_EVENT_DESTROY) {
        POBJECT Ob = Handle;
        PDEVICE Device = Ob->Address;
        MmFreePool(Device->DevicePath);
        MmFreePool(Device->DisplayName);
    }
    return STATUS_SUCCESS;
}
PDEVICE KRNLAPI PnpCreateDevice(
    UINT DeviceType,
    UINT64 DeviceCharacteristics,
    UINT16* DevicePath, 
    UINT16* DisplayName
) {


    POBJECT Object;

    NSTATUS S = ObCreateObject(NULL, &Object,
    OBJECT_PERMANENT,
    OBJECT_DEVICE,
    NULL,
    sizeof(DEVICE),
    DeviceEvtHandler
    );

    PDEVICE Device = Object->Address;
    Device->DeviceType = DeviceType;
    Device->DeviceCharacteristics = DeviceCharacteristics;

    Device->DevicePathLength = wcslen(DevicePath);
    Device->DisplayNameLength = wcslen(DisplayName);

    Device->DevicePath = KiMakeSystemNameW(DevicePath, Device->DevicePathLength);
    Device->DisplayName = KiMakeSystemNameW(DisplayName, Device->DisplayNameLength);
    Device->ObjectDescriptor = Object;

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