#include <nos/serial.h>
#include <intrin.h>
void SerialWrite(char* Msg) {
    while(*Msg) {
        __outbyte(0x3F8, *Msg);
        Msg++;
    }
    __outbyte(0x3F8, '\n');
}