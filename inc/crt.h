/*
 * The NewOperatingSystem Basic C Runtime Library :
    available for used on any platform in any operating mode

*/
#pragma once
#include <stdarg.h>

#ifdef __CRT_SRC
typedef unsigned short wchar_t;
#define RADIX_DECIMAL 10
#define RADIX_HEXADECIMAL 0x10
#define RADIX_BINARY 1
#ifndef NULL
#define NULL (void *)0
#endif
#define EXPORT __declspec(dllexport)

EXPORT char *_itoa(int value, char *buffer, int radix);
EXPORT char *_ui64toa(unsigned long long value, char *buffer, int radix);

// 128 Byte blocks
EXPORT void __fastcall XmemsetAligned(void *Address, unsigned long long Value, unsigned long long Count);
EXPORT void __fastcall XmemsetUnaligned(void *Address, unsigned long long Value, unsigned long long Count);

#pragma function(memset)
#pragma function(memcpy)
#pragma function(memcmp)
#pragma function(strlen)

#else

void __fastcall XmemsetAligned(void *Address, unsigned long long Value, unsigned long long Count);
void __fastcall XmemsetUnaligned(void *Address, unsigned long long Value, unsigned long long Count);

int vsprintf_s(
    char *buffer,
    unsigned long long sizeOfBuffer,
    const char *format,
    va_list args);

int sprintf_s(
    char *buffer,
    unsigned long long sizeOfBuffer,
    const char *format,
    ...);
unsigned long long strlen(
    const char *str);
char *_ui64toa(unsigned long long value, char *buffer, int radix);

int memcmp(
    const void *buffer1,
    const void *buffer2,
    unsigned long long count);

void *memcpy(
    void *dest,
    const void *src,
    unsigned long long count);

#ifndef max
#define max(a, b) ((a > b) ? a : b)
#endif
#ifndef min
#define min(a, b) ((a > b) ? b : a)
#endif

#endif
