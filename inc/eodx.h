/*
 * The EODX (Enhanced & Optimized Direct Graphics) Library main include file
 * Contains functions for user mode programs, for kernel mode programs include eodxsys.h instead
 * And link the program with eodxsys.dll
*/

#pragma once
#include <nosdef.h>




typedef struct _EODXINITDATA {
    UINT64 Characteristics;
    UINT32 ViewPortId;
} EODXINITDATA;

typedef void* EODXINSTANCE;
#define EODXAPI __fastcall __declspec((dllexport))



#ifndef __EODX_SYS

#endif