section .text

global SchedulerEntry

extern LocalApicAddress

; Internal Processor Descriptors
IPRSDSC equ 0xFFFFFFD000000000



; ------- STACK IS NOT 16 Byte aligned, STACK IS 8 BYTE ALIGNED ------
SchedulerEntry:
    push rbx
    push rax
    ; Read APIC ID
    mov rbx, [rel LocalApicAddress]
    mov ebx, [rbx + 0x20]
    shr ebx, 24
    shl ebx, 21
    
    mov rax, IPRSDSC
    add rbx, rax

; Save registers
    mov rbx, [rbx + 8] ; CURRENT_THREAD
    pop qword [rbx + 0x1028] ; RAX
    pop qword [rbx + 0x1030] ; RBX

    fxsave [rbx]
    pop qword [rbx + 0x1008] ; RIP
    movdqa xmm0, [rsp]
    movdqa [rbx + 0x1010], xmm0 ; RIP, CS
    movdqa xmm0, [rsp + 0x10]
    movdqa [rbx + 0x1020], xmm0 ; RFLAGS, RSP

    ; Save other General purpose registers
    mov [rbx + 0x1040], rcx
    mov [rbx + 0x1048], rdx
    mov [rbx + 0x1050], rsi
    mov [rbx + 0x1058], rdi
    mov [rbx + 0x1060], r8
    mov [rbx + 0x1068], r9
    mov [rbx + 0x1070], r10
    mov [rbx + 0x1078], r11
    mov [rbx + 0x1080], r12
    mov [rbx + 0x1088], r13
    mov [rbx + 0x1090], r14
    mov [rbx + 0x1098], r15
    ; Page Table
    mov rax, cr3
    mov [rbx + 0x10A0], rax
    ; other segments
    mov ax, gs
    shl eax, 16
    mov ax, fs
    shl rax, 16
    mov ax, es
    shl rax, 16
    mov ax, ds
    mov [rbx + 0x10A8], rax ; DS, ES, FS, GS

; call scheduling function
sub rsp, 0x50
add rsp, 0x58 ; for alignment
mov rax, rbx
; Restore registers (returned in rax)

; Restore page table
    mov rbx, [rax + 0x10A0]
    mov cr3, rbx

    mov rcx, [rax + 0x1040]
    mov rdx, [rax + 0x1048]
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

; Setup stack frame
    push qword [rax + 0x1008]
    movdqa xmm0, [rax + 0x1010]
    movdqa [rsp + 8], xmm0
    movdqa xmm0, [rax + 0x1020]
    movdqa [rsp + 0x18], xmm0

; Set timer frequency

; restore remaining registers & segments
    mov rcx, 0xfafbabe
    jmp $


    