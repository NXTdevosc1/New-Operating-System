section .text

global NosKernelEntry
global NosInitData

global memset
global memcpy
global __SystemLoadGDTAndTSS


extern NosSystemInit

; The NOS Kernel Entry Point
NosKernelEntry:
    ; Clear RFLAGS
    push 0
    popf

    mov rbp, _KernelStack
    mov rsp, _KernelStack - 0x2000

    ; Save NosInitData Pointer in RDI
    mov rbx, NosInitData
    mov [rbx], rdi

    push 0 ; Align data

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

__SystemLoadGDTAndTSS:
    ; RCX = GDTR Pointer
    lgdt [rcx]
    mov ax, 0x10
    mov fs, ax
    mov es, ax
    mov ds, ax
    mov gs, ax
    mov ss, ax
    
    mov ax, 0x18 ; TSS Offset
    ltr ax
    pop rax
    push 0x08
    push rax
    retfq



section .data

align 0x40
NosInitData dq 1

section .bss

align 0x1000
_KernelStackBase resb 0x100000
_KernelStack: