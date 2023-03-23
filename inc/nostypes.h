/*
 * NOS Types Header
*/

// Defines
#pragma once

#define IN
#define OUT
#define OPT
#define OPTIONAL
#define NOSAPI __fastcall
#define NULL (void*)0
#define TRUE 1
#define FALSE 0
// integer typedefs
typedef unsigned long long UINT64;
typedef unsigned long long ULONGLONG;
typedef long long INT64;
typedef long long LONGLONG;

typedef unsigned int UINT32;
typedef unsigned int UINT;
typedef int INT32;
typedef int INT;

typedef unsigned short UINT16;
typedef unsigned short USHORT;
typedef short INT16;
typedef short SHORT;

typedef unsigned char UINT8;
typedef unsigned char BYTE;
typedef unsigned char BOOLEAN;
typedef char INT8;

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