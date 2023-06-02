#include <nos/nos.h>
#include <nos/pnp/pnp.h>
#include <io.h>
#include <nos/ob/obutil.h>

BOOLEAN KRNLAPI IoSetInterface(
    PDEVICE Device,
    IO_INTERFACE_DESCRIPTOR* Io
) {
    UINT64 f = ExAcquireSpinLock(&Device->SpinLock);

    Device->Io.NumFunctions = Io->NumFunctions;
    Device->Io.Flags = Io->Flags;
    Device->Io.IoCallback = Io->IoCallback;
    ExReleaseSpinLock(&Device->SpinLock, f);

    return TRUE;
}

IORESULT KRNLAPI IoProtocol(
    HANDLE DeviceHandle,
    UINT Function,
    ...
) {
    POBJECT Object = ObiReferenceByHandle(DeviceHandle)->Object;

    PDEVICE Device = Object->Address;


    if(Function >= Device->Io.NumFunctions) return STATUS_INVALID_PARAMETER;
    if(Device->Io.Flags & IO_CALLBACK_SYNCHRONOUS) {
        while(_interlockedbittestandset(&Device->Io.Flags, IO_CALLBACK_SPINLOCK)) _mm_pause();
    }
    IORESULT s = Device->Io.IoCallback(DeviceHandle, Object, Function);
    _interlockedbittestandreset(&Device->Io.Flags, IO_CALLBACK_SPINLOCK);
    return s;
}
