#pragma once
#include <ddk.h>

typedef INT8 int8_t;
typedef UINT8 uint8_t;
typedef INT16 int16_t;
typedef UINT16 uint16_t;
typedef INT32 int32_t;
typedef UINT32 uint32_t;
typedef INT64 int64_t;
typedef UINT64 uint64_t;

typedef INT8 int8;
typedef UINT8 uint8;
typedef BOOLEAN Bool;
typedef UINT16 uint16;
typedef INT16 int16;
typedef UINT32 uint32;
typedef INT32 int32;
typedef UINT64 uint64;
typedef INT64 int64;
#define INLINE inline

#define SVGAPI __fastcall

#include <svga_reg.h>
#include <svga_overlay.h>
#include <svga_escape.h>
#include <svga3d_regs.h>
#include <svga3d_caps.h>
#include <svga3d_shaderdefs.h>
#include <vmmouse_defs.h>
#include <intrin.h>
typedef struct {
    PCI_DEVICE_LOCATION PciLocation;
    PDEVICE Device;
    UINT VersionId;
    UINT16 IoBase;
    UINT8* FbMem;
    UINT32* FifoMem;
    UINT VramSize;
    UINT FbSize;
    UINT FifoSize;
    UINT Capabilities;
    UINT FifoCapabilities;

    UINT32 Width;
    UINT32 Height;
    UINT32 Bpp;
    UINT32 Pitch;
} VMSVGA;

NSTATUS DevAdd(SYSTEM_DEVICE_ADD_CONTEXT* Context);

static inline UINT32 SvgaRead(IN VMSVGA* Svga, IN UINT32 Index) {
    __outdword(Svga->IoBase + SVGA_INDEX_PORT, Index);
    return __indword(Svga->IoBase + SVGA_VALUE_PORT);
}

static inline void SvgaWrite(IN VMSVGA* Svga, IN UINT32 Index, IN UINT32 Value) {
    __outdword(Svga->IoBase + SVGA_INDEX_PORT, Index);
    __outdword(Svga->IoBase + SVGA_VALUE_PORT, Value);
}

static inline BOOLEAN SvgaFifoCap(VMSVGA* Svga, UINT Mask) {
    return ((Svga->FifoMem[SVGA_FIFO_CAPABILITIES] & Mask) == Mask);
}

static inline BOOLEAN SvgaFifoRegValid(VMSVGA* Svga, UINT Reg) {
    return Svga->FifoMem[SVGA_FIFO_MIN] > (Reg << 2);
}

void SVGAPI SvgaEnable(VMSVGA* Svga);
void SVGAPI SvgaGetMode(
    VMSVGA* Svga, 
    UINT32* HorizontalResolution,
    UINT32* VerticalResolution,
    UINT32* Pitch,
    UINT32* BitsPerPixel
);

void SVGAPI SvgaSetMode(
    VMSVGA* Svga, 
    UINT32 HorizontalResolution,
    UINT32 VerticalResolution,
    UINT32 BitsPerPixel
);