#pragma once
#include <nos/nos.h>



NSTATUS __resevt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 DesiredAccess) {
    return STATUS_SUCCESS;
}