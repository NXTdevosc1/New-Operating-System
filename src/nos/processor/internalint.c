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
            SerialLog("#DIV");

            break;
        }
        case CPU_INTERRUPT_DEBUG_EXCEPTION:
        {
            SerialLog("#DBG");

            break;
        }
        case CPU_INTERRUPT_NON_MASKABLE_INTERRUPT:
        {
            SerialLog("#NMI");

            break;
        }
        case CPU_INTERRUPT_PAGE_FAULT:
        {
            SerialLog("#PF");
            break;
        }
        case CPU_INTERRUPT_GENERAL_PROTECTION_FAULT:
        {
            SerialLog("#GPF");

            break;
        }
        case CPU_INTERRUPT_DOUBLE_FAULT:
        {
            SerialLog("#DF");

            break;
        }
    }
    while(1); // TODO : Task Switch
    return InterruptStack;
}