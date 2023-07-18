#include "sddkinternal.h"


NSTATUS DllMain() {
    KDebugPrint("System Driver Developement Kit (SDDK) Entry Point.");
    return STATUS_SUCCESS;
}