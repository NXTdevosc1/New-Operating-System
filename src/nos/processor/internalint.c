#include <nos/nos.h>
#include <nos/processor/processor.h>
#include <nos/processor/ints.h>
#include <nos/processor/hw.h>

#include <intmgr.h>
void* NosInternalInterruptHandler(UINT64 InterruptNumber, void* InterruptStack) {
    INTERRUPT_ERRCODE_STACK_FRAME* ErrStack = InterruptStack;
    INTERRUPT_STACK_FRAME* StackFrame = InterruptStack;
    KDebugPrint("Internal interrupt #%u : CPU#%u , RIP : %x, CS : %x, RFLAGS : %x, RSP : %x, SS : %x",
    InterruptNumber, KeGetCurrentProcessorId(), StackFrame->InstructionPointer, StackFrame->CodeSegment,
    StackFrame->Rflags, StackFrame->StackPointer, StackFrame->StackSegment
    );

    
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
            if(KeGetCurrentProcess()->Subsystem == SUBSYSTEM_NATIVE) {
                // _disable();
                // Shutdown other cpus
                KDebugPrint("Kernel-Mode Process Crashed.");
                HwSendIpi(SYSINT_HALT, 0, IPI_NORMAL, IPI_DESTINATION_BROADCAST_ALL);
            }
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
    while(1) __halt(); // TODO : Task Switch
    return InterruptStack;
}