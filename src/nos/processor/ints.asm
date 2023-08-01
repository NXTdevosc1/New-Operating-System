section .text

extern NosInternalInterruptHandler
extern NosIrqHandler
extern NosSystemInterruptHandler
%include "task/sched.asm"

%macro DeclareIIH 1
__NosInternalInterruptHandler%1:
    push rcx
    push rdx
    mov rcx, %1 ; Interrupt Number
    lea rdx, [rsp + 0x10]
    call NosInternalInterruptHandler
    ; TODO : Task switch
    hlt
    jmp $
    pop rdx
    pop rcx
    iretq
%endmacro

%macro DeclarePtrIIH 1
dq __NosInternalInterruptHandler%1
%endmacro

_KFX times 0x1000 db 0

%macro DeclareIRQH 1
__NosIrqHandler%1:
    ; push rax
    ; push rbx
    ; mov rbx, _KFX
    ; fxsave [rbx]
    ; push rcx
    ; push rdx
    ; push rsi
    ; push rdi
    ; push r8
    ; push r9
    ; push r10
    ; push r11
    ; push r12
    ; push r13
    ; push r14
    ; push r15
    ; push rbp

    SaveTask
    push rcx
    mov rcx, %1 ; IrqNumber
    lea rdx, [rsp + 15 * 8]
    sub rsp, 0x20
    call NosIrqHandler
    add rsp, 0x20
    pop rcx
    ; mov rbx, _KFX
    ; fxrstor [rbx]
    ; pop rbp
    ; pop r15
    ; pop r14
    ; pop r13
    ; pop r12
    ; pop r11
    ; pop r10
    ; pop r9
    ; pop r8
    ; pop rdi
    ; pop rsi
    ; pop rdx
    ; pop rcx
    ; pop rbx
    ; pop rax
    ; iretq
    jmp __InternalPreemptionIrq
%endmacro

%macro DeclarePtrIRQH 1
dq __NosIrqHandler%1
%endmacro


%macro DeclareSIH 1
__NosSystemInterruptHandler%1:
    SaveTask
    push rcx
    mov rcx, %1 ; System Interrupt Number
    lea rdx, [rsp + 0x10]
    sub rsp, 0x20
    call NosSystemInterruptHandler
    add rsp, 0x20
    pop rcx
    ; These interrupts are fired through IPIs
    jmp __InternalPreemptionIrq
%endmacro

%macro DeclarePtrSIH 1
dq __NosSystemInterruptHandler%1
%endmacro

global KiInternalInterrupts
global KiIrqInterrupts
global KiSystemInterrupts

%assign i 0
%rep 32
    DeclareIIH i
    %assign i i + 1
%endrep

%assign i 0
%rep 188
    DeclareIRQH i
    %assign i i + 1
%endrep

%assign i 0
%rep 35
    DeclareSIH i
    %assign i i + 1
%endrep

KiInternalInterrupts:
    %assign i 0
    %rep 32
        DeclarePtrIIH i
        %assign i i + 1
    %endrep

KiIrqInterrupts:
    %assign i 0
    %rep 188
        DeclarePtrIRQH i
        %assign i i + 1
    %endrep

KiSystemInterrupts:
    %assign i 0
    %rep 35
        DeclarePtrSIH i
        %assign i i + 1
    %endrep