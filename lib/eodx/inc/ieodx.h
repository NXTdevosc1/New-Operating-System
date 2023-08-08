#pragma once
#include <nosdef.h>

// Describes back buffers
// Indexed by ZINDEX
typedef struct _EODXSWAPBUFFER {
    UINT64 Characteristics;
    UINT16 HorizontalResolution;
    UINT16 VerticalResolution;
    UINT Quality;
    PEODXSWAPBUFFER Previous;
    PEODXSWAPBUFFER Next;
    UINT32 Buffer[];
} EODXSWAPBUFFER, *PEODXSWAPBUFFER;

typedef struct _EODXENGINE {
    UINT64 Characteristics;
    PEODXVIEWPORT ViewPort;    
    // Triple Buffering
    UINT NumSwapBuffers;
    PEODXSWAPBUFFER SwapBuffers;
    void* __If[EODX_IIF_COUNT]; // Interface used by the EODX Subsystem
} EODXENGINE;
