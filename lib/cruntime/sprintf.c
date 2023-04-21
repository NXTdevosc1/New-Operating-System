#define __CRT_SRC
#include <crt.h>

EXPORT int sprintf_s(
   char *buffer,
   unsigned long long sizeOfBuffer,
   const char *format,
   ...
) {
    unsigned long long flen = strlen(format);
    while(*format && sizeOfBuffer) {
        if(*format == '%') {
            format++;
            char* selector = format + 1;
            switch(*selector) {
                case 'c' : {
                    // *buffer = 
                }
                case 's' : {

                }
                case 'd' : {

                }
                case 'u' : {

                }
                default: goto copybuff;
            }
        } else {
            copybuff:
            *buffer = *format;
            sizeOfBuffer--;
        }
    }
    return 0;
}