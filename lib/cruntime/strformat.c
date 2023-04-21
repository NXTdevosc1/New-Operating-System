#include <crt.h>
typedef unsigned short wchar_t;
#define RADIX_DECIMAL 10
#define RADIX_HEXADECIMAL 0x10
#define RADIX_BINARY 1
#define NULL (void*)0
#define EXPORT __declspec(dllexport)

int DllMain() {
    return 0;
}

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

EXPORT char * _itoa( int value, char *buffer, int radix ){
    
    char* retbuff = buffer;
    if (radix == RADIX_DECIMAL) { // Base 10 : Decimal
        int size = 0;
        unsigned int sizeTest = value < 0 ? -value : value;
        if(value < 0) {
            *buffer='-';
            buffer++;
        }
        while (sizeTest / 10 > 0)
        {
            sizeTest /= 10;
            size++;
        }
        int index = 0;
        while (value / 10 > 0)
        {
            char remainder = value % 10;
            value /= 10;
            buffer[size - index] = remainder + '0';
            index++;
        }
        char remainder = value % 10;
        buffer[size - index] = remainder + '0';
        buffer[size + 1] = 0;
        return retbuff;
    }
    else if (radix == RADIX_HEXADECIMAL) // Base 16 : HEX
    {
        int size = 0;
        unsigned int ValTmp = value;
        do {
            size++;
        } while((ValTmp >>= 4));
        for (int i = 0; i < size; i++)
        {
            unsigned char c = value & 0xF;
            if(c < 0xA){
                buffer[size - (i + 1)] = '0' + c;
            }else{
                buffer[size - (i + 1)] = 'A' + (c - 0xA);
            }
            value >>= 4;
        }
        buffer[size] = 0;
        return retbuff;
    }
    else if (radix == RADIX_BINARY) { // Base 1 : BINARY
        char* buff = buffer;
        *buff = '0'; // In case that value == 0
        if(!value){
            buff++;
            *buff = 0;
            return buffer;
        }
        unsigned char shift = 0;
        while (!(value & 0x80000000) && value) {
            value <<= 1;
            shift++;
        }
        shift = 32 - shift;
        while (shift--) {
            if (value & 0x80000000) {
                *buff = '1';
            }
            else *buff = '0';
            buff++;
            value <<= 1;
        }
        *buff = 0;
        return retbuff;
    }

    else return 0;
}
EXPORT char * _ltoa( long value, char *buffer, int radix ) {
    return NULL;
}
EXPORT char * _ultoa( unsigned long value, char *buffer, int radix ) {
    return NULL;
}
EXPORT char * _i64toa( long long value, char *buffer, int radix ) {
    return NULL;
}
EXPORT char * _ui64toa( unsigned long long value, char *buffer, int radix ){
    char* retbuff = buffer;
    if (radix == RADIX_DECIMAL) { // Base 10 : Decimal
        int size = 0;
        unsigned long long sizeTest = value < 0 ? -value : value;
        if(value < 0) {
            *buffer='-';
            buffer++;
        }
        while (sizeTest / 10 > 0)
        {
            sizeTest /= 10;
            size++;
        }
        int index = 0;
        while (value / 10 > 0)
        {
            char remainder = value % 10;
            value /= 10;
            buffer[size - index] = remainder + '0';
            index++;
        }
        char remainder = value % 10;
        buffer[size - index] = remainder + '0';
        buffer[size + 1] = 0;
        return retbuff;
    }
    else if (radix == RADIX_HEXADECIMAL) // Base 16 : HEX
    {
        int size = 0;
        unsigned long long ValTmp = value;
        do {
            size++;
        } while((ValTmp >>= 4));
        for (int i = 0; i < size; i++)
        {
            unsigned char c = value & 0xF;
            if(c < 0xA){
                buffer[size - (i + 1)] = '0' + c;
            }else{
                buffer[size - (i + 1)] = 'A' + (c - 0xA);
            }
            value >>= 4;
        }
        buffer[size] = 0;
        return retbuff;
    }
    else if (radix == RADIX_BINARY) { // Base 1 : BINARY
        char* buff = buffer;
        *buff = '0'; // In case that value == 0
        if(!value){
            buff++;
            *buff = 0;
            return buffer;
        }
        unsigned char shift = 0;
        while (!(value & 0x80000000) && value) {
            value <<= 1;
            shift++;
        }
        shift = 64 - shift;
        while (shift--) {
            if (value & 0x80000000) {
                *buff = '1';
            }
            else *buff = '0';
            buff++;
            value <<= 1;
        }
        *buff = 0;
        return retbuff;
    }

    else return 0;
}

EXPORT wchar_t * _itow( int value, wchar_t *buffer, int radix ){
    return NULL;
}
EXPORT wchar_t * _ltow( long value, wchar_t *buffer, int radix ){
    return NULL;
}
EXPORT wchar_t * _ultow( unsigned long value, wchar_t *buffer, int radix ){
    return NULL;
}
EXPORT wchar_t * _i64tow( long long value, wchar_t *buffer, int radix ){
    return NULL;
}
EXPORT wchar_t * _ui64tow( unsigned long long value, wchar_t *buffer, int radix ){
    return NULL;
}
