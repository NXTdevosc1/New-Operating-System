#include <ddk.h>


NSTATUS NOSENTRY DriverEntry(
    PDRIVER Driver
) {
    KeRegisterEventHandler(
        
    )
    return STATUS_SUCCESS;
}