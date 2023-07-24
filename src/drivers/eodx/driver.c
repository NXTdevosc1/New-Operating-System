#include <eodxdriver.h>



PDEVICE EodxDevice;

EODXIOENTRY EodxIo[EODX_NUMFUNCTIONS] = {0};




IORESULT __fastcall EodxAsyncIoCallback(IOPARAMS) {
    if(Function >= EODX_NUMFUNCTIONS) KeRaiseException(STATUS_INVALID_PARAMETER);
    if(EodxIo[Function].NumArgs != NumParameters) KeRaiseException(STATUS_INVALID_PARAMETER);
    
    DEVICE_IO_CALLBACK cb = (DEVICE_IO_CALLBACK)EodxIo[Function].Callback;
    return cb(DeviceHandle, Function, NumParameters, Parameters);
}



NSTATUS NOSAPI DriverEntry(PDRIVER Driver) {
    KDebugPrint("EODX Driver Startup. DriverID %u", Driver->DriverId);
    

    EodxDevice = KeCreateDevice(
        VIRTUAL_DEVICE,
        0,
        L"Enhanced And Optimized Direct Graphics (EODX) 1.0",
        NULL
    );

    IO_INTERFACE_DESCRIPTOR Io = {0};
    Io.NumFunctions = EODX_NUMFUNCTIONS;
    Io.IoCallback = EodxAsyncIoCallback;

    IoSetInterface(EodxDevice, &Io);

    PEODX_ADAPTER SoftwareAdapter = iEodxInitSoftwareGraphicsProcessor();


    return STATUS_SUCCESS;
}