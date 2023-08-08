section .text

global __AltSchedule ; Called via INT 0xF1

extern LocalApicAddress
extern Schedule 

global __InternalPreemptionCpuInt

%include "task/sched.asm"

; Internal Processor Descriptors

; ------- STACK IS NOT 16 Byte aligned, STACK IS 8 BYTE ALIGNED ------
__AltSchedule:


    SaveTask

__InternalPreemptionCpuInt:
; call scheduling function
push 0
push rcx
sub rsp, 0x100
mov rdx, 1
call Schedule ; RCX = Internl CPU Data
add rsp, 0x100
pop rcx
sub rsp, 8
; Restore page table
    mov rbx, [rax + 0x10A8]
    mov rdx, cr3
    cmp rdx, rbx
    je .NoTlbFlush
    mov rax, 0xbabafefe
    jmp $
    mov cr3, rbx
.NoTlbFlush:

; Restore registers (returned in rax)
    mov rsi, [rax + 0x1050]
    mov rdi, [rax + 0x1058]
    mov r8, [rax + 0x1060]
    mov r9, [rax + 0x1068]
    mov r10, [rax + 0x1070]
    mov r11, [rax + 0x1078]
    mov r12, [rax + 0x1080]
    mov r13, [rax + 0x1088]
    mov r14, [rax + 0x1090]
    mov r15, [rax + 0x1098]
    mov rbp, [rax + 0x10A0]
; Setup stack frame
    push qword [rax + 0x1008]
    movdqa xmm0, [rax + 0x1010]
    movdqa [rsp + 8], xmm0
    movdqa xmm0, [rax + 0x1020]
    movdqa [rsp + 0x18], xmm0
; restore segments
    mov rbx, [rax + 0x10B0]
    mov ds, bx
    shr rbx, 16
    mov es, bx
    shr rbx, 16
    mov fs, bx
    shr rbx, 16
    mov gs, bx
; Set timer frequency, and EOI
    mov rbx, [rel LocalApicAddress]
    mov rdx, [rcx + 0x30] ; Timer freq
    shr rdx, SCHEDSHIFT ; 1024 Task switches / s
    mov [rbx + 0x380], edx ; Initial Count
    ;mov dword [rbx + 0xB0], 0 ; Eoi
; Restore remaining registers
    mov rdx, [rax + 0x1048]
    mov rcx, [rax + 0x1040]
    mov rbx, [rax + 0x1038]
    fxrstor [rax]
    mov rax, [rax + 0x1030]
; Switch Tasks
    o64 iret

