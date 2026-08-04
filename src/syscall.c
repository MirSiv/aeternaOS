#include "syscall.h"
#include "serial.h"
#include <stdint.h>

// Внешняя ссылка на ассемблерный обработчик
extern void syscall_handler_asm(void);

// Обработчик системных вызовов на C
void syscall_handler(struct syscall_frame *frame) {
    // Детальное логирование для отладки через COM1
    klog("\r\n[SYSCALL] === SYSCALL ENTRY ===\r\n");
    klog("[SYSCALL] RAX (syscall num): ");
    klog_dec(frame->rax);
    klog("\r\n[SYSCALL] Registers:\r\n");
    klog("  RDI: "); klog_hex(frame->rdi); klog("\r\n");
    klog("  RSI: "); klog_hex(frame->rsi); klog("\r\n");
    klog("  RDX: "); klog_hex(frame->rdx); klog("\r\n");
    klog("  RCX: "); klog_hex(frame->rcx); klog("\r\n");
    klog("  R8:  "); klog_hex(frame->r8);  klog("\r\n");
    klog("  R9:  "); klog_hex(frame->r9);  klog("\r\n");
    klog("  R10: "); klog_hex(frame->r10); klog("\r\n");
    klog("  R11: "); klog_hex(frame->r11); klog("\r\n");
    
    switch (frame->rax) {
        case SYS_EXIT:
            klog("[SYSCALL] -> SYS_EXIT (code=");
            klog_dec(frame->rdi);
            klog(")\r\n");
            // Завершение процесса
            // frame->rdi содержит код выхода
            break;
            
        case SYS_READ:
            klog("[SYSCALL] -> SYS_READ (fd=");
            klog_dec(frame->rdi);
            klog(", buf=");
            klog_hex(frame->rsi);
            klog(", count=");
            klog_dec(frame->rdx);
            klog(")\r\n");
            // Чтение из файла/descriptor
            // frame->rdi = fd, frame->rsi = buf, frame->rdx = count
            frame->rax = 0; // вернуть 0 байт (заглушка)
            break;
            
        case SYS_WRITE:
            klog("[SYSCALL] -> SYS_WRITE (fd=");
            klog_dec(frame->rdi);
            klog(", buf=");
            klog_hex(frame->rsi);
            klog(", count=");
            klog_dec(frame->rdx);
            klog(")\r\n");
            // Запись в файл/descriptor
            // frame->rdi = fd, frame->rsi = buf, frame->rdx = count
            frame->rax = frame->rdx; // вернуть количество байт (заглушка)
            break;
            
        case SYS_OPEN:
            klog("[SYSCALL] -> SYS_OPEN (path=");
            klog_hex(frame->rdi);
            klog(", flags=");
            klog_hex(frame->rsi);
            klog(")\r\n");
            // Открытие файла
            // frame->rdi = pathname, frame->rsi = flags
            frame->rax = -1; // ошибка (заглушка)
            break;
            
        case SYS_CLOSE:
            klog("[SYSCALL] -> SYS_CLOSE (fd=");
            klog_dec(frame->rdi);
            klog(")\r\n");
            // Закрытие файла
            // frame->rdi = fd
            frame->rax = 0; // успех (заглушка)
            break;
            
        case SYS_MMAP:
            klog("[SYSCALL] -> SYS_MMAP\r\n");
            // Отображение памяти
            frame->rax = 0; // заглушка
            break;
            
        case SYS_MUNMAP:
            klog("[SYSCALL] -> SYS_MUNMAP\r\n");
            // Отключение отображения памяти
            frame->rax = 0; // заглушка
            break;
            
        case SYS_BRK:
            klog("[SYSCALL] -> SYS_BRK\r\n");
            // Изменение размера кучи
            frame->rax = 0; // заглушка
            break;
            
        case SYS_FORK:
            klog("[SYSCALL] -> SYS_FORK\r\n");
            // Создание дочернего процесса
            frame->rax = -1; // не реализовано
            break;
            
        case SYS_EXECVE:
            klog("[SYSCALL] -> SYS_EXECVE\r\n");
            // Выполнение программы
            frame->rax = -1; // не реализовано
            break;
            
        case SYS_EXIT_GROUP:
            klog("[SYSCALL] -> SYS_EXIT_GROUP\r\n");
            // Завершение группы потоков
            break;
            
        default:
            klog("[SYSCALL] -> UNKNOWN SYSCALL (");
            klog_dec(frame->rax);
            klog(")\r\n");
            frame->rax = -1; // неверный номер системного вызова
            break;
    }
    
    klog("[SYSCALL] Returning RAX: ");
    klog_hex(frame->rax);
    klog("\r\n[SYSCALL] === SYSCALL EXIT ===\r\n");
}

// Инициализация MSR регистров для SYSCALL/SYSRET
void syscall_init(void) {
    klog("[SYSCALL] initializing syscall interface...\r\n");
    
    // Настройка MSR_LSTAR (0xC0000081) - адрес обработчика
    uint64_t lstar = (uint64_t)syscall_handler_asm;
    uint32_t lstar_low = (uint32_t)(lstar & 0xFFFFFFFF);
    uint32_t lstar_high = (uint32_t)((lstar >> 32) & 0xFFFFFFFF);
    
    klog("[SYSCALL] LSTAR (handler addr): ");
    klog_hex(lstar);
    klog("\r\n");
    
    __asm__ volatile (
        "mov $0xC0000081, %%ecx\n\t"  // MSR_LSTAR
        "mov %0, %%eax\n\t"
        "mov %1, %%edx\n\t"
        "wrmsr\n\t"
        :
        : "r"(lstar_low), "r"(lstar_high)
        : "eax", "ecx", "edx"
    );
    
    // Настройка MSR_STAR (0xC0000080) - сегменты CS/SS
    // [47:32] = CS Ring 0, [63:48] = SS Ring 3
    // Для нашего GDT:
    //   KERNEL_CS = 0x08 (Ring 0)
    //   USER_CS = 0x1B (Ring 3)
    //   USER_SS = 0x23 (Ring 3)
    // STAR[47:32] = KERNEL_CS
    // STAR[63:48] = USER_SS
    uint64_t star = ((uint64_t)0x23 << 48) | ((uint64_t)0x08 << 32);
    uint32_t star_low = (uint32_t)(star & 0xFFFFFFFF);
    uint32_t star_high = (uint32_t)((star >> 32) & 0xFFFFFFFF);
    
    __asm__ volatile (
        "mov $0xC0000080, %%ecx\n\t"  // MSR_STAR
        "mov %0, %%eax\n\t"
        "mov %1, %%edx\n\t"
        "wrmsr\n\t"
        :
        : "r"(star_low), "r"(star_high)
        : "eax", "ecx", "edx"
    );

    // Настройка MSR_FMASK (0xC0000084) - флаги, которые будут очищены при SYSCALL
    // Обычно маскируем IF (interrupt flag) и другие привилегированные флаги
    uint64_t fmask = 0x00200200;  // RF и DF
    uint32_t fmask_low = (uint32_t)(fmask & 0xFFFFFFFF);
    uint32_t fmask_high = (uint32_t)((fmask >> 32) & 0xFFFFFFFF);
    
    klog("[SYSCALL] FMASK: ");
    klog_hex(fmask);
    klog("\r\n");

    __asm__ volatile (
        "mov $0xC0000084, %%ecx\n\t"  // MSR_FMASK
        "mov %0, %%eax\n\t"
        "mov %1, %%edx\n\t"
        "wrmsr\n\t"
        :
        : "r"(fmask_low), "r"(fmask_high)
        : "eax", "ecx", "edx"
    );

    klog("[SYSCALL] MSR registers configured\r\n");
    klog("[SYSCALL] syscall interface ready\r\n");
}
