#include <nos/nos.h>
#include <nos/serial.h>
static inline void SerialSend(char value) {
    __outbyte(SERIAL_COM1, value);
}

BOOLEAN SerialInitialized = FALSE;

void SerialInitCom1() {
    SerialInitialized = TRUE;
    __outbyte(SERIAL_COM1 + 1, 0);
    __outbyte(SERIAL_COM1 + 3, 0x80);
    __outbyte(SERIAL_COM1, 0x03);
    __outbyte(SERIAL_COM1 + 1, 0);
    __outbyte(SERIAL_COM1 + 3, 0x03);
    __outbyte(SERIAL_COM1 + 2, 0xC7);
    __outbyte(SERIAL_COM1 + 4, 0x0B);
    __outbyte(SERIAL_COM1 + 4, 0x1E);
    __outbyte(SERIAL_COM1, 0xAE);

    if(__inbyte(SERIAL_COM1) != 0xAE) {
        KDebugPrint("Failed to init COM1");
    }
    __outbyte(SERIAL_COM1 + 4, 0xF);
    KDebugPrint("COM1 Initialized Successfully");
}

void SerialWait() {
    // while(__inbyte(SERIAL_COM1 + 5) & 0x20) _mm_pause();
}

static char format[0x100];

void SerialWrite(char* Msg) {
    RFPROCESSOR p = KeGetCurrentProcessor();
    UINT64 pid = 0;
    char* hdr;
    if(p) {
        hdr = "Processor#%d : ";
        pid = p->Id.ProcessorId;
    } else {
        hdr = "Boot Processor : ";
    }
    sprintf_s(format, 0x100, hdr, pid);
    char* s = format;
    while(*s) {
        SerialSend(*s);
        s++;
    }
    if(!SerialInitialized) {
        SerialInitCom1();
    }
    while(*Msg) {
        SerialSend(*Msg);
        Msg++;
    }
    // Send to all serial ports
    SerialSend('\n');
    SerialWait();
}