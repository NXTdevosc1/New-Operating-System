#define __CRT_SRC
#include <crt.h>

void *memset(
    void *dest,
    char c,
    unsigned long long count)
{
    if (!(count & 7))
        __stosq(dest, c, count >> 3);
    else if (!(count & 3))
        __stosd(dest, c, count >> 2);
    else
        __stosb(dest, c, count);
    return dest;
}
void *memcpy(
    void *dest,
    const void *src,
    unsigned long long count)
{
    char *d = dest;
    const char *s = src;
    for (unsigned long long i = 0; i < count; i++, d++, s++)
        *d = *s;
    return dest;
}

int memcmp(
    const void *buffer1,
    const void *buffer2,
    unsigned long long count)
{
    while (count)
    {
        char b1 = *(char *)buffer1, b2 = *(char *)buffer2;
        if (b1 < b2)
        {
            return -1;
        }
        else if (b1 > b2)
        {
            return 1;
        }
        ((char *)buffer1)++;
        ((char *)buffer2)++;
        count--;
    }
    return 0;
}

int Support = -1;

extern void _SSEMemsetA(void *a, unsigned long long v, unsigned long long c);
extern void _AVXMemsetA(void *a, unsigned long long v, unsigned long long c);
extern void _SSEMemsetU(void *a, unsigned long long v, unsigned long long c);
extern void _AVXMemsetU(void *a, unsigned long long v, unsigned long long c);

void(__fastcall *XMemsetAFunctions[])(void *, unsigned long long, unsigned long long) = {
    _SSEMemsetA, _AVXMemsetA};

void(__fastcall *XMemsetUFunctions[])(void *, unsigned long long, unsigned long long) = {
    _SSEMemsetU, _AVXMemsetU};

typedef struct _CPUID_DATA
{
    unsigned int eax, ebx, ecx, edx;
} CPUID_DATA;
#include <intrin.h>

void CheckSupportLevel()
{
    // checks for AVX, TODO : AVX512, TODO : enhanced movb
    CPUID_DATA Cpuid;
    __cpuid(&Cpuid, 1);
    if (Cpuid.ecx & (1 << 28))
    {
        // AVX Supported
        Support = 1;
    }
    else
    {
        Support = 0; // SSE2
    }
}
EXPORT void __fastcall XmemsetAligned(void *Address, unsigned long long Value, unsigned long long Count)
{
    if (Support == -1)
        CheckSupportLevel();

    XMemsetAFunctions[Support](Address, Value, Count);
}
EXPORT void __fastcall XmemsetUnaligned(void *Address, unsigned long long Value, unsigned long long Count)
{
    if (Support == -1)
        CheckSupportLevel();

    XMemsetUFunctions[Support](Address, Value, Count);
}
