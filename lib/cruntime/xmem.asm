bits 64
section .text

global _SSEMemsetA
global _SSEMemsetU
global _AVXMemsetA
global _AVXMemsetU

_SSEMemsetA:
    mov rax, 0xcaca1
    jmp $
    ret

_SSEMemsetU:
mov rax, 0xcaca2
    jmp $
    ret

; RCX = Addr, RDX = Val, R8 = Count (128 Byte blocks)
_AVXMemsetA:
    push rdx
    vbroadcastsd ymm0, [rsp]
    add rsp, 8
.loop:
    test r8, r8
    jz .end
    vmovdqa [rcx], ymm0
    vmovdqa [rcx + 0x20], ymm0
    vmovdqa [rcx + 0x40], ymm0
    vmovdqa [rcx + 0x60], ymm0
    dec r8
    add rcx, 0x80
    jmp .loop

.end:
    ret

_AVXMemsetU:
mov rax, 0xcaca4
    jmp $
    ret