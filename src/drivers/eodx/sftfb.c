#include <eodxdriver.h>

__declspec(dllimport) NOS_INITDATA* __fastcall KiGetInitData();



NSTATUS __fastcall SoftwareAdapterCallback(
    PEODX_ADAPTER Adapter,
    void* Context,
    UINT Command,
    PVOID Payload
) {
    FRAME_BUFFER_DESCRIPTOR* fb = Context;
    if(Command == EODXHW_CMD_WRITEBUFFER) {

    } else if(Command == EODXHW_CMD_READBUFFER) {

    } else if (Command == EODXHW_CMD_CLEARBUFFER) {
        _Memset128A_32(fb->BaseAddress, 0, fb->FbSize >> 4);
    }
    return STATUS_UNSUPPORTED;
}

PEODX_ADAPTER iEodxInitSoftwareGraphicsProcessor() {
    FRAME_BUFFER_DESCRIPTOR* FrameBuffer = &KiGetInitData()->FrameBuffer;
    KDebugPrint("Software Frame Buffer at %x %ux%u", FrameBuffer->BaseAddress, FrameBuffer->HorizontalResolution, FrameBuffer->VerticalResolution);
    
    PEODX_ADAPTER SoftwareAdapter = iEodxCreateDisplayAdapter(
        L"Software Display Adapter",
        ADAPTER_SOFTWARE,
        0x100, // 1.0
        FrameBuffer,
        SoftwareAdapterCallback
    );



    // Clear the front buffer so that nothing weird displays
    SoftwareAdapter->Callback(SoftwareAdapter, SoftwareAdapter->Context, EODXHW_CMD_CLEARBUFFER, NULL);
    KDebugPrint("EODX Software adapter created successfully");

    return SoftwareAdapter;
}