section .text

global SchedulerEntry

extern LocalApicAddress

; Internal Processor Descriptors
IPRSDSC equ 0xFFFFFFD000000000

SchedulerEntry:
    ; Read APIC ID
    mov rbx, [rel LocalApicAddress]
    mov ebx, [rbx + 0x20]
    shr ebx, 24
    shl ebx, 21
    
    mov rcx, IPRSDSC
    add rbx, rcx

; Save registers
    ;mov []

; call scheduling function

; Restore registers (returned in rax)

    mov rax, 0xbabe
    hlt
    jmp $


    