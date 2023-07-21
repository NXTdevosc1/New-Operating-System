#include <nos/nos.h>
#include <nos/processor/processor.h>
#include <nos/processor/ints.h>
#include <nos/processor/hw.h>

#include <intmgr.h>

extern void DrawRect(UINT x, UINT y, UINT Width, UINT Height, UINT Color);
extern void KRNLAPI KiDrawDebugRect(UINT8 DbgStage);

void* NosInternalInterruptHandler(UINT64 InterruptNumber, void* InterruptStack) {
    INTERRUPT_STACK_FRAME* StackFrame = InterruptStack;
    INTERRUPT_ERRCODE_STACK_FRAME* ErrStack = InterruptStack;


    if(KeGetCurrentProcess()->Subsystem == SUBSYSTEM_NATIVE) {
        _disable();
        // Shutdown other cpus
        // KDebugPrint("Registers : rax=%x rbx=%x rcx=%x rdx=%x rsi=%x rdi=%x");
        // KDebugPrint("r8=%x r9=%x r10=%x r11=%x r12=%x r13=%x r14=%x r15=%x");
        if(LocalApicAddress)
            HwSendIpi(SYSINT_HALT, 0, IPI_NORMAL, IPI_DESTINATION_BROADCAST_OTHERS);

        for(int i = 0;i<0x1000;i++) _mm_pause();
    KDebugPrint("___________________________________________________________");
    KDebugPrint("Internal interrupt #%u : CPU#%u , RIP : %x, CS : %x, RFLAGS : %x, RSP : %x, SS : %x",
    InterruptNumber, KeGetCurrentProcessorId(), StackFrame->InstructionPointer, StackFrame->CodeSegment,
    StackFrame->Rflags, StackFrame->StackPointer, StackFrame->StackSegment
    );
        KDebugPrint("Kernel-Mode Process Crashed.");
        DrawRect(0, 0, 500, 500, 0xFFFF);
    }
    
    switch(InterruptNumber) {
        case CPU_INTERRUPT_DIVIDED_BY_0:
        {
            SerialLog("#DIV");
        KiDrawDebugRect(0);

            break;
        }
        case CPU_INTERRUPT_DEBUG_EXCEPTION:
        {
            SerialLog("#DBG");
        KiDrawDebugRect(1);

            break;
        }
        case CPU_INTERRUPT_NON_MASKABLE_INTERRUPT:
        {
            SerialLog("#NMI");
        KiDrawDebugRect(2);

            break;
        }
        case CPU_INTERRUPT_PAGE_FAULT:
        {
        KiDrawDebugRect(3);
            
            KDebugPrint("#PF ErrCode : %d", ErrStack->ErrorCode);
            KDebugPrint("CR2 : %x", __readcr2());
            if(!(ErrStack->ErrorCode & 1)) {
                KDebugPrint("Page not present");
            }
            if(ErrStack->ErrorCode & 2) {
                KDebugPrint("Write access not allowed");
            }
            if(ErrStack->ErrorCode & 0x10) {
                KDebugPrint("Execution in Execute Disabled Page");
            }
            break;
        }
        case CPU_INTERRUPT_GENERAL_PROTECTION_FAULT:
        {
        KiDrawDebugRect(4);

            KDebugPrint("#GPF ErrCode : %d", ErrStack->ErrorCode);
            break;
        }
        case CPU_INTERRUPT_DOUBLE_FAULT:
        {
        KiDrawDebugRect(5);

            SerialLog("#DF");

            break;
        }
    }
        
    while(1) __halt(); // TODO : Task Switch
    return InterruptStack;
}