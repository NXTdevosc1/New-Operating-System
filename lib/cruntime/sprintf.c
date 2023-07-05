#define __CRT_SRC
#include <crt.h>
#include <stdarg.h>
#define sprintf_ret {*buffer = 0; return 0;}
#define sprintf_cpbuffer(_buff, _character) {*_buff = _character; _buff++;sizeOfBuffer--;if(!sizeOfBuffer) sprintf_ret;}

EXPORT int vsprintf_s(
   char *buffer,
   unsigned long long sizeOfBuffer,
   const char *format,
   va_list args
) {
    if(!sizeOfBuffer) return -1;
    unsigned int NumArgs = 0;
    unsigned long long flen = strlen(format);
    sizeOfBuffer--; // End character
    while(*format && sizeOfBuffer) {
        if(*format == '%') {
            format++;
            const char* selector = format;
            int SelSize = 0;
            for(;selector[SelSize] && 
            (
            !(selector[SelSize] >= 'a' && selector[SelSize] <= 'z') &&
            !(selector[SelSize] >= 'A' && selector[SelSize] <= 'Z')
            )
             ;SelSize++);

            char Operation = selector[SelSize];

            format+=SelSize+1;
            switch(Operation) {
                case 'c' :{
                    sprintf_cpbuffer(buffer, va_arg(args, char));
                    NumArgs++;
                    break;
                }
                case 's' :
                case 'a' :
                 {
                    char* bf = va_arg(args, char*);
                    NumArgs++;
                    while(*bf) {
                        sprintf_cpbuffer(buffer, *bf);
                        bf++;
                    }
                    break;
                }
                case 'd' : {
                    // write 32 bit signed integer
                    int num = va_arg(args, int);
                    NumArgs++;
                    char* b = buffer;
                    buffer = _itoa(num, buffer, 10);
                    if(buffer - b >= sizeOfBuffer) sprintf_ret;
                    sizeOfBuffer-= buffer - b;
                    break;
                }
                case 'u' : {
                    unsigned long long num = va_arg(args, unsigned long long);
                    NumArgs++;
                    char* b = buffer;
                    buffer = _ui64toa(num, buffer, 10);
                    if(buffer - b >= sizeOfBuffer) sprintf_ret;
                    sizeOfBuffer-= buffer - b;
                    break;
                }
                case 'x' :
                case 'X' :
                 {
                    unsigned long long num = va_arg(args, unsigned long long);
                    NumArgs++;
                    char* b = buffer;
                    buffer = _ui64toa(num, buffer, 0x10);
                    if(buffer - b >= sizeOfBuffer) sprintf_ret;
                    sizeOfBuffer-= buffer - b;
                    break;
                }
                case 'l':
                {
                    SelSize++;
                    format++;
                    char selector2 = selector[SelSize];
                    if(selector2 == 's') {
                        short* bf = va_arg(args, short*);
                    NumArgs++;
                    while(*bf) {
                        sprintf_cpbuffer(buffer, *bf);
                        bf++;
                    }
                    }

                    break;
                }
                default: {
                    format -= SelSize + 2;
                    goto copybuff;
                }
            }
        } else {
            copybuff:
            *buffer = *format;
            buffer++;
            sizeOfBuffer--;
            format++;
        }
    }
    *buffer = 0;
    return 0;
}



EXPORT int sprintf_s(
   char *buffer,
   unsigned long long sizeOfBuffer,
   const char *format,
   ...
) {
    va_list args;
    va_start(args, format);
    int s = vsprintf_s(buffer, sizeOfBuffer, format, args);
    va_end(args);
    return s;
}