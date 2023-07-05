#include <nos/nos.h>
#include <nos/pnp/pnp.h>
#include <nos/ob/obutil.h>
#include <ob.h>
NSTATUS DeviceEvtHandler(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access) {
    if(Event == OBJECT_EVENT_DESTROY) {
        POBJECT Ob = Handle;
        PDEVICE Device = Ob->Address;
        MmFreePool(Device->DisplayName);
    }
    return STATUS_SUCCESS;
}
PDEVICE KRNLAPI PnpCreateDevice(
    UINT DeviceType,
    UINT64 DeviceCharacteristics,
    PNPID* PnpId,
    UINT16* DisplayName
){


    POBJECT Object;

    NSTATUS S = ObCreateObject(NULL, &Object,
    OBJECT_PERMANENT,
    OBJECT_DEVICE,
    NULL,
    sizeof(DEVICE),
    DeviceEvtHandler
    );

    PDEVICE Device = Object->Address;

    Device->PnpId.Parent = PnpId->Parent;
    Device->PnpId.DeviceLocation.Raw = PnpId->DeviceLocation.Raw;


    Device->DeviceType = DeviceType;
    Device->DeviceCharacteristics = DeviceCharacteristics;

    Device->DisplayNameLength = wcslen(DisplayName);

    Device->DisplayName = KiMakeSystemNameW(DisplayName, Device->DisplayNameLength);
    Device->ObjectDescriptor = Object;

    return Device;
}


BOOLEAN KRNLAPI PnpGetDeviceName(PDEVICE Device, UINT16* DeviceName) {
    memcpy(DeviceName, Device->DisplayName, (Device->DisplayNameLength + 1) << 1);    
    return TRUE;
}

BOOLEAN KRNLAPI PnpDeleteDevice(PDEVICE Device) {
    HANDLE h;
    if(NERROR(ObOpenHandleByAddress(KernelProcess, Device, OBJECT_DEVICE, 0, &h))) return FALSE;

    ObCloseHandle(KernelProcess, h);
    MmFreePool(Device->DisplayName);

    // Do not force because operations may be still done on the device
    if(!ObDestroyObject(Device->ObjectDescriptor, 0)) return FALSE;

    return TRUE;

}