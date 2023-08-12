#define __CRT_SRC
#include <crt.h>

unsigned long long strlen(
   const char *str
) {
    unsigned long long len = 0;
    while(*str) {
        str++;
        len++;
    }
    return len;
}
