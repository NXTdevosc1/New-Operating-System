section .text

extern NosInternalInterruptHandler
extern NosIrqHandler
extern NosSystemInterruptHandler

%macro DeclareIIH 1
__NosInternalInterruptHandler%1:
    push rcx
    mov rcx, %1 ; Interrupt Number
    call NosInternalInterruptHandler
    pop rcx
    iretq
%endmacro

%macro DeclarePtrIIH 1
dq __NosInternalInterruptHandler%1
%endmacro

%macro DeclareIRQH 1
__NosIrqHandler%1:
    push rcx
    mov rcx, %1 ; IrqNumber
    call NosIrqHandler
    iretq
%endmacro

%macro DeclarePtrIRQH 1
dq __NosIrqHandler%1
%endmacro

%macro DeclareSIH 1
__NosSystemInterruptHandler%1:
    push rcx
    mov rcx, %1 ; System Interrupt Number
    call NosSystemInterruptHandler
    pop rcx
    iretq
%endmacro

%macro DeclarePtrSIH 1
dq __NosSystemInterruptHandler%1
%endmacro

global KiInternalInterrupts
global KiIrqInterrupts
global KiSystemInterrupts

%assign i 0
    %rep 31
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
    %rep 31
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