#include <nos/serial.h>

static inline void SerialSend(char value) {
    __outbyte(SERIAL_COM1, value);
}

void SerialWrite(char* Msg) {
    while(*Msg) {
        SerialSend(*Msg);
        Msg++;
    }
    // Send to all serial ports
    SerialSend('\n');
}