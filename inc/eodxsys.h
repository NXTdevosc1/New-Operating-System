#pragma once

#define __EODX_SYS
#include <eodx.h>

#define EODX_IIF_COUNT 10 // Internal Interface Count

typedef struct _EODXVIEWPORT {
    UINT64 ViewPortId;
    UINT64 GraphicsDeviceId; // - 1 = CPU
    UINT VendorId;
    HANDLE Driver;
    UINT16 MajorVersion, MinorVersion;

    UINT16 HorizontalResolution;
    UINT16 VerticalResolution;

    void* Interface[EODX_IIF_COUNT]; // Interface used by the EODX Subsystem
} EODXVIEWPORT, *PEODXVIEWPORT;

typedef enum _EODXHW_COMMAND {
// Eodx Front Buffer Accesses
    EODXHW_CMD_READBUFFER,
    EODXHW_CMD_WRITEBUFFER,
    EODXHW_CMD_CLEARBUFFER
} EODXHW_COMMAND;

// EODX Payloads

// Payload for read/write commands
typedef struct _EODXP_READWRITEBUFFER {
    UINT16 x, y, width, height;
    // Buffer should be filled
    UINT32* Buffer; // 8 Bit RGBA Buffer
} EODXP_READWRITEBUFFER, *PEODXP_READWRITEBUFFER;
