#include <nos/processor/internal.h>
#include <nos/processor/ints.h>
#include <intmgr.h>
void* NosInternalInterruptHandler(UINT64 InterruptNumber, void* InterruptStack) {
    INTERRUPT_ERRCODE_STACK_FRAME* ErrStack = InterruptStack;
    INTERRUPT_STACK_FRAME* StackFrame = InterruptStack;
    KDebugPrint("Internal interrupt #%u , RIP : %x, CS : %x, RFLAGS : %x, RSP : %x, SS : %x",
    InterruptNumber, StackFrame->InstructionPointer, StackFrame->CodeSegment,
    StackFrame->Rflags, StackFrame->StackPointer, StackFrame->StackSegment
    );

    _disable();
    
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
            KDebugPrint("#PF ErrCode : %d", ErrStack->ErrorCode);
            break;
        }
        case CPU_INTERRUPT_GENERAL_PROTECTION_FAULT:
        {
            KDebugPrint("#GPF ErrCode : %d", ErrStack->ErrorCode);
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