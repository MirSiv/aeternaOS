#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// Структура фрейма системного вызова (сохранённые регистры при входе из Ring 3)
struct syscall_frame {
    // Регистры, сохранённые syscall_handler
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;  // содержит RFLAGS после SYSCALL
    uint64_t r10;  // содержит RIP пользователя (возврат)
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;  // содержит RIP пользователя (копия)
    uint64_t rax;  // номер системного вызова
    
    // Сегментные регистры (для отладки/расширения)
    uint64_t fs;
    uint64_t gs;
} __attribute__((packed));

// Инициализация MSR регистров для SYSCALL/SYSRET
void syscall_init(void);

// Обработчик системных вызовов (вызывается из ассемблера)
// Принимает указатель на фрейм сохранённых регистров
void syscall_handler_c(struct syscall_frame *frame);

// Ассемблерная обёртка обработчика (вызывается напрямую из ASM)
void syscall_handler(void);

// Номера системных вызовов
#define SYS_EXIT        0
#define SYS_READ        1
#define SYS_WRITE       2
#define SYS_OPEN        3
#define SYS_CLOSE       4
#define SYS_MMAP        9
#define SYS_MUNMAP      11
#define SYS_BRK         12
#define SYS_FORK        57
#define SYS_EXECVE      59
#define SYS_EXIT_GROUP  231

#endif
