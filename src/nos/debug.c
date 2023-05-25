#include <nos/nos.h>
char __debug_fmt[0x1000];
void KRNLAPI KDebugPrint(IN char* Message, ...) {
    va_list args;
    va_start(args, Message);
    vsprintf_s(__debug_fmt, 0x1000, Message, args);
    va_end(args);
    SerialLog(__debug_fmt);
}