section .text

global SchedulerEntry

SchedulerEntry:
    mov rax, 0xbabe
    hlt
    jmp $


    