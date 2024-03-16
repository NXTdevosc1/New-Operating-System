#include <nos/nos.h>
#include <nos/pnp/pnp.h>
#include <nos/ob/obutil.h>
#include <ob.h>
NSTATUS DeviceEvtHandler(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access)
{
    if (Event == OBJECT_EVENT_DESTROY)
    {
        POBJECT Ob = Handle;
        PDEVICE Device = Ob->Address;
        KlFreePool(Device->DisplayName);
    }
    return STATUS_SUCCESS;
}
PDEVICE KRNLAPI KeCreateDevice(
    UINT DeviceType,
    UINT64 DeviceCharacteristics,
    UINT16 *DisplayName,
    void *Context)
{

    POBJECT Object;

    NSTATUS S = ObCreateObject(NULL, &Object,
                               OBJECT_PERMANENT,
                               OBJECT_DEVICE,
                               NULL,
                               sizeof(DEVICE),
                               DeviceEvtHandler);

    PDEVICE Device = Object->Address;

    Device->DeviceType = DeviceType;
    Device->DeviceCharacteristics = DeviceCharacteristics;

    Device->DisplayNameLength = wcslen(DisplayName);

    Device->DisplayName = KiMakeSystemNameW(DisplayName, Device->DisplayNameLength);
    Device->ObjectDescriptor = Object;
    Device->Context = Context;

    return Device;
}

BOOLEAN KRNLAPI KeGetDeviceName(PDEVICE Device, UINT16 *DeviceName, UINT8 *NameLength)
{
    *NameLength = Device->DisplayNameLength;
    memcpy(DeviceName, Device->DisplayName, (Device->DisplayNameLength + 1) << 1);
    return TRUE;
}

BOOLEAN KRNLAPI KeEnumerateDevices(IN OPT PDEVICE ParentDevice, IN PDEVICE *Device, IN OPT UINT16 *DeviceName, IN BOOLEAN AbsoluteName, IN UINT64 *EnumValue)
{
    POBJECT devobj;
    if (DeviceName)
    {
        UINT64 ev = *EnumValue;
        UINT8 NameLen = wcslen(DeviceName);

        while ((ev = ObEnumerateObjects(ParentDevice ? ParentDevice->ObjectDescriptor : NULL, OBJECT_DEVICE, &devobj, NULL, ev)) != 0)
        {
            PDEVICE dev = devobj->Address;
            if (AbsoluteName && dev->DisplayNameLength == NameLen && memcmp(dev->DisplayName, DeviceName, NameLen << 1) == 0)
            {
                *EnumValue = ev;
                *Device = dev;
                return TRUE;
            }
            else if (dev->DisplayNameLength >= NameLen && memcmp(dev->DisplayName, DeviceName, NameLen << 1) == 0)
            {
                *EnumValue = ev;
                *Device = dev;
                return TRUE;
            }
        }
        return FALSE;
    }
    UINT64 ev = ObEnumerateObjects(ParentDevice ? ParentDevice->ObjectDescriptor : NULL, OBJECT_DEVICE, &devobj, NULL, *EnumValue);
    if (ev == 0)
        return FALSE;
    *Device = devobj->Address;
    *EnumValue = ev;
    return TRUE;
}

BOOLEAN KRNLAPI KeRemoveDevice(PDEVICE Device)
{
    HANDLE h;
    if (NERROR(ObOpenHandleByAddress(KernelProcess, Device, OBJECT_DEVICE, 0, &h)))
        return FALSE;

    ObCloseHandle(KernelProcess, h);
    KlFreePool(Device->DisplayName);

    // Do not force because operations may be still done on the device
    if (!ObDestroyObject(Device->ObjectDescriptor, 0))
        return FALSE;

    return TRUE;
}

POBJECT KRNLAPI KeGetDeviceObjectHeader(PDEVICE Device)
{
    return Device->ObjectDescriptor;
}