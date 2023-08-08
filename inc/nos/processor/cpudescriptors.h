#include <nosdef.h>



#define IDT_CALL_GATE 12
#define IDT_INTERRUPT_GATE 14
#define IDT_TRAP_GATE 15

#pragma pack(push, 1)
typedef struct {
    UINT16 Limit;
    void* Address;
} SYSTEM_DESCRIPTOR;
#pragma pack(pop)