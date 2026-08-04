; syscall.asm - Обработчик системных вызовов через SYSCALL/SYSRET
; Перехват системных вызовов из пользовательского режима (Ring 3)

bits 64

section .text

global syscall_handler_asm
extern syscall_handler

; ============================================================================
; syscall_handler_asm - точка входа для системных вызовов
; Вызывается инструкцией SYSCALL из Ring 3
;
; На входе (согласно ABI SYSCALL):
;   rax - номер системного вызова
;   rdi, rsi, rdx, r10, r8, r9 - аргументы
;   rcx - RIP возврата (пользовательский режим)
;   r11 - RFLAGS после выполнения SYSCALL
;
; После SYSCALL процессор:
;   - переключается на Ring 0 стек (из MSR_IA32_KERNEL_GSBASE или TSS.RSP0)
;   - устанавливает CS и SS в соответствии с MSR_LSTAR
;   - сохраняет RIP возврата в RCX
;   - сохраняет RFLAGS в R11
; ============================================================================

syscall_handler_asm:
    ; Сохраняем регистры пользователя на стеке ядра
    push r15
    push r14
    push r13
    push r12
    push r11        ; RFLAGS
    push r10        ; RIP пользователя (копия 1)
    push r9
    push r8
    push rdi
    push rsi
    push rbp
    push rbx
    push rdx
    push rcx        ; RIP пользователя (копия 2)
    push rax        ; номер системного вызова
    
    ; Сохраняем сегментные регистры (могут понадобиться)
    push fs
    push gs
    
    ; Выравниваем стек до 16 байт перед вызовом функции C
    mov rbp, rsp
    and rsp, -16
    sub rsp, 8      ; выравнивание под SysV ABI
    
    ; Передаём указатель на фрейм в RDI (первый аргумент по SysV ABI)
    mov rdi, rsp
    call syscall_handler
    
    ; Восстанавливаем стек
    mov rsp, rbp
    
    ; Восстанавливаем сегментные регистры
    pop gs
    pop fs
    
    ; Восстанавливаем регистры (кроме RAX, который содержит результат)
    pop rcx         ; RIP возврата
    pop rdx
    pop rbx
    pop rbp
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11         ; RFLAGS
    pop r12
    pop r13
    pop r14
    pop r15
    ; RAX остаётся без изменений (результат системного вызова)
    
    ; Возврат в пользовательский режим через SYSRET
    ; SYSRET использует:
    ;   RCX - RIP возврата
    ;   R11 - RFLAGS возврата
    ;   MSR_IA32_STAR[47:32] - CS для Ring 3
    ;   MSR_IA32_STAR[63:48] - SS для Ring 3
    sysretq

section .data

; ============================================================================
; Инициализация MSR регистров для SYSCALL/SYSRET
; ============================================================================

global syscall_msr_init
syscall_msr_init:
    ; Эта функция должна вызываться из C кода для настройки MSR
    ; Реализация здесь для справки:
    ;
    ; MSR_LSTAR (0xC0000081) - адрес syscall_handler_asm
    ; MSR_STAR (0xC0000080) - сегменты CS/SS для Ring 0 и Ring 3
    ; MSR_FMASK (0xC0000084) - маска флагов, которые будут очищены
    ;
    ; wrmsr требует:
    ;   ecx - номер MSR
    ;   eax - младшие 32 бита значения
    ;   edx - старшие 32 бита значения
    ret

section .rodata

; Константы MSR
MSR_LSTAR     equ 0xC0000081    ; Long Mode System Call Target Address Register
MSR_STAR      equ 0xC0000080    ; System Call Target Address
MSR_FMASK     equ 0xC0000084    ; System Call Flag Mask
MSR_KERNELGSB equ 0xC0000102    ; Kernel GS Base

; Селекторы сегментов (должны совпадать с GDT)
KERNEL_CS     equ 0x08          ; Селектор кода ядра (Ring 0)
KERNEL_DS     equ 0x10          ; Селектор данных ядра (Ring 0)
USER_CS       equ 0x1B          ; Селектор кода пользователя (Ring 3)
USER_DS       equ 0x23          ; Селектор данных пользователя (Ring 3)
