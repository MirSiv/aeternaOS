section .multiboot2
align 8

multiboot_start:
    dd 0xE85250D6
    dd 0
    dd multiboot_end - multiboot_start
    dd -(0xE85250D6 + 0 + (multiboot_end - multiboot_start))

    ; address tag
    align 8
    dw 2
    dw 0
    dd 24

    dd multiboot_start

    extern kernel_header_start
    dd kernel_header_start

    extern kernel_code_end
    dd kernel_code_end

    extern kernel_header_end
    dd kernel_header_end

    ; entry tag
    align 8
    dw 3
    dw 0
    dd 12
    dd _start

    ; end tag
    align 8
    dw 0
    dw 0
    dd 8

multiboot_end:

; !!! text section !!!

section .text
bits 32

global _start

_start:
    cli

    mov esp, stack_top

    ; save multiboot pointer
    mov esi, ebx

    call check_multiboot

    call check_cpuid

    call check_long_mode

    call setup_page_tables

    ; CR3 = PML4
    mov eax, pml4
    mov cr3, eax

    ; enable PAE
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; enable long mode
    mov ecx, 0xC0000080

    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, (1 << 31) | 1
    mov cr0, eax

    ; load GDT
    lgdt [gdt64.pointer]


    ; preserve multiboot ptr
    mov edi, esi

    ; jump to long mode
    jmp 0x08:long_mode_start

; !!! checks !!!

check_multiboot:
    cmp eax, 0x36D76289
    jne .fail
    ret

.fail:
    mov al, '0'
    jmp error

check_cpuid:
    pushfd
    pop eax

    mov ecx, eax

    xor eax, 1 << 21

    push eax
    popfd

    pushfd
    pop eax

    push ecx
    popfd

    cmp eax, ecx
    je .fail

    ret

.fail:
    mov al, '1'
    jmp error

check_long_mode:
    mov eax, 0x80000000
    cpuid

    cmp eax, 0x80000001
    jb .fail

    mov eax, 0x80000001
    cpuid

    test edx, 1 << 29
    jz .fail

    ret

.fail:
    mov al, '2'
    jmp error

; !!! page tables !!!

setup_page_tables:

    ; clear tables
    mov edi, pml4
    mov ecx, (4096 * 3) / 4
    xor eax, eax
    rep stosd

    ; PML4 -> PDPT
    mov eax, pdpt
    or eax, 0b11

    mov dword [pml4], eax
    mov dword [pml4 + 4], 0

    ; PDPT -> PD
    mov eax, pd
    or eax, 0b11

    mov dword [pdpt], eax
    mov dword [pdpt + 4], 0

    ; map 64MB using 2MB pages
    xor ecx, ecx

.map_pd:

    mov eax, ecx
    shl eax, 21

    or eax, 0b10000011

    mov dword [pd + ecx * 8], eax
    mov dword [pd + ecx * 8 + 4], 0

    inc ecx
    cmp ecx, 32
    jne .map_pd

    ret

; !!! err !!!

error:
    mov dword [0xB8000], 0x4F524F45
    mov dword [0xB8004], 0x4F3A4F52
    mov byte  [0xB8008], al

.hang:
    hlt
    jmp .hang

; !!! long mode !!!

bits 64

extern kernel_main

long_mode_start:


    ; reload segments
    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax


    ; zero extend multiboot ptr
    mov edi, edi

    ; aligned stack
    mov rsp, stack_top
    and rsp, -16

    ; SysV ABI alignment
    sub rsp, 8

    call kernel_main

.halt:
    hlt
    jmp .halt

; !!! gdt/tss helpers !!!

global gdt_flush

gdt_flush:
    lgdt [rdi]

    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    push 0x08
    lea rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    ret

global tss_flush

tss_flush:
    mov ax, 0x18
    ltr ax
    ret

; !!! isr !!!

extern exception_handler

%macro ISR_NOERRCODE 1
global isr_stub_%1
isr_stub_%1:
    push qword 0
    push qword %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr_stub_%1
isr_stub_%1:
    push qword %1
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29
ISR_ERRCODE   30
ISR_NOERRCODE 31

isr_common_stub:

    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp

    mov rbp, rsp
    and rsp, -16
    sub rsp, 8

    call exception_handler

    mov rsp, rbp

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    add rsp, 16

    iretq

; !!! isr table !!!

global isr_stub_table

align 8

; !!! hardware irq handlers !!!

extern timer_handler
extern keyboard_handler

global irq0
irq0:
    push qword 0  ; "код ошибки" (заглушка для сохранения структуры)
    push qword 32 ; номер прерывания (вектор таймера)
    
    ; сохраняем все регистры (формируем interrupt_frame)
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp    ; передаем указатель на фрейм первым аргументом в Си
    mov rbp, rsp    ; запоминаем стек
    and rsp, -16    ; выравниваем по 16 байт
    sub rsp, 8

    call timer_handler

    mov rsp, rbp    ; восстанавливаем стек
    
    ; восстанавливаем регистры
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    add rsp, 16    
    iretq

global irq1
irq1:
    push qword 0
    push qword 33 
    
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    mov rbp, rsp
    and rsp, -16
    sub rsp, 8

    call keyboard_handler

    mov rsp, rbp
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    add rsp, 16
    iretq

isr_stub_table:

%assign i 0
%rep 32
    dq isr_stub_%+i
%assign i i + 1
%endrep

; !!! rodata !!!

section .rodata
align 8

gdt64:
    dq 0x0000000000000000
    dq 0x00AF9A000000FFFF
    dq 0x00AF92000000FFFF

.pointer:
    dw $ - gdt64 - 1
    dq gdt64

; !!! bss !!!

section .bss

align 4096

pml4:
    resb 4096

pdpt:
    resb 4096

pd:
    resb 4096

align 16

stack_bottom:
    resb 16384

stack_top:

section .note.GNU-stack noalloc noexec nowrite progbits