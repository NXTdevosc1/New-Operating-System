#pragma once
#include <nosdef.h>

#define SYSTEM_CONTROL_PORT_A 0x92
#define SYSTEM_CONTROL_PORT_B 0x61

PVOID KRNLAPI KeFindSystemFirmwareTable(
    IN char* FirmwareTableId,
    IN OPT GUID* EfiFirmwareTableGuid
);