#include <intrin.h>

void (__fastcall *EnterSystem)() = (void*)-0x1000;

void __declspec(dllexport) __cdecl DebugOutput(const char *Message) {
    
    _setjmp(EnterSystem);
}
