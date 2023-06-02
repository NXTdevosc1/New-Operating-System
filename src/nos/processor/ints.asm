section .text

extern NosInternalInterruptHandler
extern NosIrqHandler
extern NosSystemInterruptHandler

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

%macro DeclareIRQH 1
__NosIrqHandler%1:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push rbp

    mov rcx, %1 ; IrqNumber
    lea rdx, [rsp + 15 * 8]

    call NosIrqHandler
    pop rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ;mov rax, 0xfafa
    ;mov rbx, [rsp]
    ;mov rcx, [rsp + 8]
    ;mov rdx, [rsp + 0x10]
    ;mov rsi, [rsp + 0x18]
    ;mov rdi, [rsp + 0x20]
    ;jmp $
    iretq
%endmacro

%macro DeclarePtrIRQH 1
dq __NosIrqHandler%1
%endmacro

%macro DeclareSIH 1
__NosSystemInterruptHandler%1:
    push rcx
    push rdx
    mov rcx, %1 ; System Interrupt Number
    lea rdx, [rsp + 0x10]
    call NosSystemInterruptHandler
    pop rdx
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