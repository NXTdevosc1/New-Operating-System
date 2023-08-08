#pragma once
#include <nosdef.h>


NSTATUS KRNLAPI KeLoadImage(
    void* ImageBuffer,
    UINT64 ImageSize,
    BOOLEAN OperatingMode,
    POBJECT ParentObject,
    // If KERNEL_MODE then _Out will hold the value of the entry point address
    // otherwise it will return a pointer to the process
    // if Image type is a DLL, then the caller must set the parent process address in _Out
    void** _Out
);

NSTATUS KiInitLibraries(PDRIVER Driver);