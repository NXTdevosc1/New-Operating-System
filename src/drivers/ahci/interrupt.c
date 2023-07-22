#include <ahci.h>

NSTATUS __cdecl AhciInterruptHandler(INTERRUPT_HANDLER_DATA* Interrupt) {
    PAHCI Ahci = Interrupt->Context;
    KDebugPrint("Ahci %x Interrupt", Ahci);
    return STATUS_SUCCESS;
}