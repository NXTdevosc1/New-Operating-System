section .text

extern HwMultiProcessorEntry
extern __kcr3

extern MmAllocatePhysicalMemory

global HwInitTrampoline
global __end_HwInitTrampoline
align 0x1000

[BITS 16]

HwInitTrampoline:


    mov ax, cs ; set segments to the base address of the trampoline
    mov ss, ax
    mov ds, ax
    mov es, ax

    mov esp, 0x6500 ; ss + esp
    mov ebp, esp

; Clear EFLAGS
    push dword 0
    popfd

; Setup Control registers
    mov eax, (1 << 5) | (1 << 7) ; Physical Addess Extension & Page Global Enable
    mov cr4, eax

    xor eax, eax
    mov cr0, eax

; Set page table
    xor eax, eax
    mov ax, ds
    shl eax, 4 ; get physical start
    add eax, 0x1000
    mov cr3, eax

; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    mov ecx, 0xC0000080

    or eax, 1 << 8 ; Set long mode enable
    wrmsr
    
    mov eax, 1 | (1 << 31) ; Protected Mode Enable & Set Paging Enable Bit
    mov cr0, eax


; Set gdt and idt
    lgdt [0x4000]
    lidt [0x4010]



    ; Construct the address to __LongModeStartup
    xor eax, eax
    mov ax, cs
    shl eax, 4

    mov esi, eax ; Set base variable

    add eax, 0x800
    
    push dword 0x08
    push eax
    retfd

times 0x800 - ($ - $$) db 0
[BITS 64]
__LongModeStartup:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov gs, ax
    mov fs, ax

; Set the allocated stack by the kernel
    mov rsp, [rsi + 0x6008]
    mov rbp, rsp

; Enable NX before using the kernel page table
    mov ecx, 0xC0000080 ; IA32_EFER
    rdmsr
    mov ecx, 0xC0000080
    or eax, 1 << 11 ; Enable NX Bit
    xor edx, edx
    wrmsr
; Switch to the kernel mode page table
    mov rbx, [rsi + 0x6000]
    mov cr3, rbx

    mov rbx, HwMultiProcessorEntry
    jmp rbx


times 0x1000 - ($ - $$) db 0
__INITAP_PAGETABLE: ; +0x1000
    dq 0x2003 ; To be rebased by kernel
times 511 dq 0
    dq 0x3003 ; To be rebased by kernel
times 511 dq 0
    dq 0x83 ; 2 MB Page
    dq 0x200083
    times 510 dq 0

__INITAP_GDTR: ; + 0x4000
    dw __INITAP_GDTE - __INITAP_GDT - 1
    dq 0x4020 ; To be rebased by kernel
    dw 0
    dd 0
__INITAP_IDTR: ; + 0x4010
    dw 0
    dq 0
    dw 0
    dd 0
__INITAP_GDT: ; + 0x4020
     .NULL dq 0
    .Code:
        dw 0
        dw 0
        db 0
        db 10011010b
        db 1010b << 4 ; Limit | Flags << 4
        db 0
    .Data:
        dw 0
        dw 0
        db 0
        db 10010010b
        db 1000b << 4 ; Limit | Flags << 4
        db 0
__INITAP_GDTE:
times 0x6000 - ($ - $$) db 0
__KPAGE_TABLE dq 0 ; Here the kernel will put the address of its page table
__KSTACK dq 0 ; Here the kernel will put the address of the stack for the processor
times 0x8000 - ($ - $$) db 0