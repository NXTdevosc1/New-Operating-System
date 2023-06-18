#include <loader.h>

/*
 * BlInitBootGraphics
 * This function sets the Graphics Output to Native Mode and claims display information
*/
void BlInitBootGraphics() {
	EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsProtocol;
	// Check for G.O.P Support
	EFI_STATUS Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void**)&GraphicsProtocol);
	if(EFI_ERROR(Status)) {
		Print(L"Graphics Output Protocol is not supported.\n");
		gBS->Exit(gImageHandle, EFI_UNSUPPORTED, 0, NULL);
	}
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* ModeInfo;
	UINTN szInfo;
	Status = GraphicsProtocol->QueryMode(GraphicsProtocol, GraphicsProtocol->Mode == NULL ? 0 : GraphicsProtocol->Mode->Mode, &szInfo, &ModeInfo);
	if(Status == EFI_NOT_STARTED) {
		if(EFI_ERROR(GraphicsProtocol->SetMode(GraphicsProtocol, 0)) || !GraphicsProtocol->Mode || !GraphicsProtocol->Mode->FrameBufferBase) {
			Print(L"Failed to set video mode.\n");
			gBS->Exit(gImageHandle, EFI_UNSUPPORTED, 0, NULL);
		}
	}

	NosInitData.FrameBuffer.BaseAddress = (void*)GraphicsProtocol->Mode->FrameBufferBase;
	NosInitData.FrameBuffer.FbSize = GraphicsProtocol->Mode->FrameBufferSize;
	NosInitData.FrameBuffer.HorizontalResolution = ModeInfo->HorizontalResolution;
	NosInitData.FrameBuffer.VerticalResolution = ModeInfo->VerticalResolution;
	NosInitData.FrameBuffer.Pitch = ModeInfo->PixelsPerScanLine * 4;
}