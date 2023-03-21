section .text

global NosKernelEntry

; The NOS Kernel Entry Point
NosKernelEntry:
    mov rax, 0xcafebabe
    jmp $
    hlt