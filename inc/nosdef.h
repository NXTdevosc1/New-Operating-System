/*
 * NOS Types Header
*/

// Defines
#pragma once
#include <nstatus.h>
#include <immintrin.h>
#include <xmmintrin.h>
#include <zmmintrin.h>

#define IN
#define OUT
#define OPT
#define OPTIONAL
#define PREALLOCATED
#define NOSINTERNAL __fastcall
#ifndef KRNLAPI
#define NSYSAPI __declspec(dllimport) __fastcall // NSYSAPI is both accessible in user mode or kernel mode
#define KRNLAPI __declspec(dllimport) __fastcall
#endif

#define NOSENTRY __declspec(noreturn) __cdecl

#define ZeroMemory(__ptr, __NumBytes) memset((void*)(__ptr), 0, __NumBytes) 
#define ObjZeroMemory(__ptr) memset((void*)__ptr, 0, sizeof(*__ptr))
#define ConvertToPages(NumBytes) ((NumBytes & 0xFFF) ? ((NumBytes >> 12) + 1) : (NumBytes))
#define AlignForward(_val, _align) (((UINT64)(_val) & ((_align) - 1)) ? ((UINT64)(_val) + (_align) - ((UINT64)(_val) & ((_align) - 1))) : ((UINT64)(_val)))
#define AlignBackward(_val, _align) ((UINT64)(_val) & ~((_align) - 1))
#define ExcessBytes(Value, Align) (((UINT64)Value & (Align - 1)) ? (Align - ((UINT64)Value & (Align - 1))) : (0))

#ifndef NULL
#define NULL (void*)0
#endif
#define TRUE 1
#define FALSE 0
// integer typedefs
typedef unsigned long long UINT64;
typedef unsigned long long ULONGLONG;
typedef UINT64 QWORD;
typedef long long INT64;
typedef long long LONGLONG;

typedef unsigned int UINT32;
typedef unsigned int UINT;
typedef UINT32 DWORD;
typedef int INT32;
typedef int INT;
typedef long LONG;
typedef unsigned long ULONG;


typedef unsigned short UINT16;
typedef unsigned short USHORT;
typedef UINT16 WORD;
typedef short INT16;
typedef short SHORT;

typedef unsigned char UINT8;
typedef unsigned char BYTE;
typedef unsigned char BOOLEAN;
typedef char INT8;
typedef UINT8 UCHAR;
typedef INT8 CHAR;

typedef void* PVOID;

typedef struct {
  UINT32    Data1;
  UINT16    Data2;
  UINT16    Data3;
  UINT8     Data4[8];
} GUID;

typedef void* HANDLE;

#define INVALID_HANDLE (HANDLE)((UINT64)-1)
typedef struct _CPUID_DATA {
  UINT32 eax, ebx, ecx, edx;
} CPUID_DATA;

#ifndef __NOSKXPNP

typedef struct _DEVICE DEVICE, *PDEVICE;

#endif

static inline void _Memset128A_32(void* _addr, const UINT32 val, UINT64 count) {
    volatile __m128* addr = _addr;
    
    
    
    __m128 v;
    v.m128_u32[0] = val;
    v.m128_u32[1] = val;
    v.m128_u32[2] = val;
    v.m128_u32[3] = val;

    
    do {
      *addr = v;
      addr++;
    } while(count--);
}
static inline void _Memset128U_32(__unaligned void* _addr, const float val, UINT64 count) {
    volatile __unaligned __m128* addr = _addr;
    
    
    
    __m128 v;
    v.m128_u32[0] = val;
    v.m128_u32[1] = val;
    v.m128_u32[2] = val;
    v.m128_u32[3] = val;

    
    do {
      *addr = v;
      addr++;
    } while(count--);
}

typedef volatile struct _EPROCESS EPROCESS, *PEPROCESS;
typedef volatile struct _ETHREAD ETHREAD, *PETHREAD;

typedef struct _DRIVER DRIVER, *PDRIVER;
typedef struct _OBJECT_DESCRIPTOR *POBJECT;
// Generally System IO Helpers for quick parameter conversion
#define UINT32VOID (UINT32)(UINT64)
#define INT32VOID (INT32)(UINT64)
#define UINT16VOID (UINT16)(UINT64)
#define INT16VOID (INT16)(UINT64)
#define UINT8VOID (UINT8)(UINT64)
#define INT8VOID (INT8)(UINT64)

typedef NSTATUS (__fastcall *REMOTE_EXECUTE_ROUTINE)(void* Context);
