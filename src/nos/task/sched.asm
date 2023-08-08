IPRSDSC equ 0xFFFFFFD000000000

extern LocalApicAddress

; Executes EOI
extern __InternalPreemptionIrq

; Does not execute EOI
extern __InternalPreemptionCpuInt

SCHEDSHIFT equ 8

%macro SaveTask 0
     push rcx
    push rbx
    push rax
    ; Read APIC ID
    mov rbx, [rel LocalApicAddress]
    mov ebx, [rbx + 0x20]
    shr ebx, 24
    shl ebx, 21
    
    mov rax, IPRSDSC
    add rbx, rax

    mov rcx, rbx ; RCX Contains INTERNAL_CPU_DATA


; Save registers



    mov rbx, [rbx + 8] ; CURRENT_THREAD
    pop qword [rbx + 0x1030] ; RAX
    pop qword [rbx + 0x1038] ; RBX
    pop qword [rbx + 0x1040] ; RCX

    fxsave [rbx]
    pop qword [rbx + 0x1008] ; RIP
    movdqa xmm0, [rsp]
    movdqa [rbx + 0x1010], xmm0 ; RIP, CS
    movdqa xmm0, [rsp + 0x10]
    movdqa [rbx + 0x1020], xmm0 ; RFLAGS, RSP

    ; Save other General purpose registers
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
    mov [rbx + 0x10A0], rbp
    ; Page Table
    mov rax, cr3
    mov [rbx + 0x10A8], rax
    ; other segments
    mov ax, gs
    shl eax, 16
    mov ax, fs
    shl rax, 16
    mov ax, es
    shl rax, 16
    mov ax, ds
    mov [rbx + 0x10B0], rax ; DS, ES, FS, GS

%endmacro