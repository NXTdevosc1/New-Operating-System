section .text

global NosKernelEntry
global NosInitData
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

section .data

align 0x40
NosInitData dq 1