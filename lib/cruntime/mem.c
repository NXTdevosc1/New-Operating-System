#define __CRT_SRC
#include <crt.h>

EXPORT void *memset(
   void *dest,
   char c,
   unsigned long long count
) {
    if(!(count & 7)) __stosq(dest, c, count >> 3);
    else if(!(count & 3)) __stosd(dest, c, count >> 2);
    else __stosb(dest, c, count);
    return dest;
}
EXPORT void *memcpy(
   void *dest,
   const void *src,
   unsigned long long count
) {
    while(count) {
        *(char*)dest = *(char*)src;
        ((char*)dest)++;
        ((char*)src)++;
        count--;
    }
    return dest;
}

int memcmp(
   const void *buffer1,
   const void *buffer2,
   unsigned long long count
) {
    while(count) {
        char b1 = *(char*)buffer1, b2 = *(char*)buffer2;
        if(b1 < b2) {
            return -1;
        } else if(b1 > b2) {
            return 1;
        }
        ((char*)buffer1)++;
        ((char*)buffer2)++;
        count--;
    }
    return 0;
}