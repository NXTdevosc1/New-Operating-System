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
    UINT8 NumParameters,
    void** Parameters
) {

    POBJECT ObHeader;
    PDEVICE Device = ObGetObjectByHandle(DeviceHandle, &ObHeader, OBJECT_DEVICE);
    if(!Device) {
        // Crash the process
        KeRaiseException(STATUS_INVALID_PARAMETER);
    }

    PETHREAD Thread = KeGetCurrentThread();
    if(NumParameters) {
        Parameters = KeConvertPointer(Thread->Process, Parameters);
        if(!Parameters) KeRaiseException(STATUS_INVALID_PARAMETER);

        memcpy((void*)Thread->IoParameters, (void*)Parameters, NumParameters * 8);
    }

    if(Device->Io.Flags & IO_CALLBACK_SYNCHRONOUS) {
        KDebugPrint("SYNCHRONOUS IO");
        while(1);
    }

    
    Thread->RunningIo.IoDevice = Device;
    Thread->RunningIo.Function = Function;
    IORESULT res = Device->Io.IoCallback(DeviceHandle, Function, NumParameters, (void**)Thread->IoParameters);
    Thread->RunningIo.IoDevice = NULL;
    return res;
}
