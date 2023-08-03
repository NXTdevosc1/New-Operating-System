#include <nos/nos.h>
char __debug_fmt[0x1000];
__declspec(align(0x1000)) static SPINLOCK Lock = 0;
void KRNLAPI KDebugPrint(IN char* Message, ...) {
    // PROCESSOR* Processor;

    UINT64 f = ExAcquireSpinLock(&Lock);
    va_list args;
    va_start(args, Message);
    vsprintf_s(__debug_fmt, 0xFF0, Message, args);
    va_end(args);
    SerialLog(__debug_fmt);
    // if(BootProcessor && Processor->State != PROCESSOR_STATE_INTERRUPT) {
        ExReleaseSpinLock(&Lock, f);
    // }
}

// void KRNLAPI KDebugPrint(IN char* Message, ...) {
//     // __KDebugPrint("______________________________________");
//     SerialLog(Message);
//     // __KDebugPrint("vals %x %x %x %x", *(UINT64*)(&Message + 8), *(UINT64*)(&Message + 16), *(UINT64*)(&Message + 24), *(UINT64*)(&Message + 32));
//     // __KDebugPrint("______________________________________");


// }