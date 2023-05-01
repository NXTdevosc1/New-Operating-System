#include <nos/processor/internal.h>
#include <nos/processor/ints.h>

void* NosInternalInterruptHandler(UINT64 InterruptNumber, void* InterruptStack) {
    char* returnstack = InterruptStack;
    SerialLog("Internal Interrupt!");
    char bf[100];
    _ui64toa(InterruptNumber, bf, 10);
    SerialLog(bf);
    
    switch(InterruptNumber) {
        case CPU_INTERRUPT_DIVIDED_BY_0:
        {
            break;
        }
        case CPU_INTERRUPT_DEBUG_EXCEPTION:
        {
            break;
        }
        case CPU_INTERRUPT_NON_MASKABLE_INTERRUPT:
        {
            break;
        }
        case CPU_INTERRUPT_PAGE_FAULT:
        {

            break;
        }
        case CPU_INTERRUPT_GENERAL_PROTECTION_FAULT:
        {
            break;
        }
        case CPU_INTERRUPT_DOUBLE_FAULT:
        {
            break;
        }
    }
    while(1); // TODO : Task Switch
    return InterruptStack;
}