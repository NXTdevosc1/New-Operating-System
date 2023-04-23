section .text

global NosKernelEntry
global NosInitData

global memset
global memcpy
extern NosSystemInit

; The NOS Kernel Entry Point
NosKernelEntry:
    ; Clear RFLAGS
    push 0
    popf

    ; Save NosInitData Pointer in RDI
    mov rbx, NosInitData
    mov [rbx], rdi


    jmp NosSystemInit
    hlt

; RCX = Dest, DL = Value, R8 = Count
memset:
    push rdi
    mov al, dl
    mov rdi, rcx
    mov rcx, r8
    rep stosb
    pop rdi
    ret
; RCX = Dest, RDX = Src, R8 = Count
memcpy:
    push rsi
    push rdi
    mov rsi, rdx
    mov rdi, rcx
    mov rcx, r8
    rep movsb
    pop rdi
    pop rsi
    ret
section .data

align 0x40
NosInitData dq 1