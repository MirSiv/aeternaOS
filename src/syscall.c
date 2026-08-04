#include "syscall.h"
#include "serial.h"
#include <stdint.h>

// Внешняя ссылка на ассемблерный обработчик
extern void syscall_handler_asm(void);

// Диспетчер системных вызовов на C (вызывается из ассемблерного обработчика)
void syscall_handler_c(struct syscall_frame *frame) {
    // Логирование для отладки
    klog("[SYSCALL] intercepted syscall: ");
    
    switch (frame->rax) {
        case SYS_EXIT:
            klog("SYS_EXIT\n");
            // Завершение процесса
            // frame->rdi содержит код выхода
            break;
            
        case SYS_READ:
            klog("SYS_READ\n");
            // Чтение из файла/descriptor
            // frame->rdi = fd, frame->rsi = buf, frame->rdx = count
            frame->rax = 0; // вернуть 0 байт (заглушка)
            break;
            
        case SYS_WRITE:
            klog("SYS_WRITE\n");
            // Запись в файл/descriptor
            // frame->rdi = fd, frame->rsi = buf, frame->rdx = count
            frame->rax = frame->rdx; // вернуть количество байт (заглушка)
            break;
            
        case SYS_OPEN:
            klog("SYS_OPEN\n");
            // Открытие файла
            // frame->rdi = pathname, frame->rsi = flags
            frame->rax = -1; // ошибка (заглушка)
            break;
            
        case SYS_CLOSE:
            klog("SYS_CLOSE\n");
            // Закрытие файла
            // frame->rdi = fd
            frame->rax = 0; // успех (заглушка)
            break;
            
        case SYS_MMAP:
            klog("SYS_MMAP\n");
            // Отображение памяти
            frame->rax = 0; // заглушка
            break;
            
        case SYS_MUNMAP:
            klog("SYS_MUNMAP\n");
            // Отключение отображения памяти
            frame->rax = 0; // заглушка
            break;
            
        case SYS_BRK:
            klog("SYS_BRK\n");
            // Изменение размера кучи
            frame->rax = 0; // заглушка
            break;
            
        case SYS_FORK:
            klog("SYS_FORK\n");
            // Создание дочернего процесса
            frame->rax = -1; // не реализовано
            break;
            
        case SYS_EXECVE:
            klog("SYS_EXECVE\n");
            // Выполнение программы
            frame->rax = -1; // не реализовано
            break;
            
        case SYS_EXIT_GROUP:
            klog("SYS_EXIT_GROUP\n");
            // Завершение группы потоков
            break;
            
        default:
            klog("UNKNOWN (");
            // Вывод номера системного вызова (упрощённо)
            klog(")\n");
            frame->rax = -1; // неверный номер системного вызова
            break;
    }
}

// Ассемблерный обработчик (обёртка)
void syscall_handler(void) {
    struct syscall_frame *frame = (struct syscall_frame *)__builtin_frame_address(0);
    syscall_handler_c(frame);
}

// Инициализация MSR регистров для SYSCALL/SYSRET
void syscall_init(void) {
    klog("[SYSCALL] initializing syscall interface...\n");
    
    // Настройка MSR_LSTAR (0xC0000081) - адрес обработчика
    uint64_t lstar = (uint64_t)syscall_handler_asm;
    uint32_t lstar_low = (uint32_t)(lstar & 0xFFFFFFFF);
    uint32_t lstar_high = (uint32_t)((lstar >> 32) & 0xFFFFFFFF);
    
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
    
    __asm__ volatile (
        "mov $0xC0000084, %%ecx\n\t"  // MSR_FMASK
        "mov %0, %%eax\n\t"
        "mov %1, %%edx\n\t"
        "wrmsr\n\t"
        :
        : "r"(fmask_low), "r"(fmask_high)
        : "eax", "ecx", "edx"
    );
    
    klog("[SYSCALL] MSR registers configured\n");
    klog("[SYSCALL] syscall interface ready\n");
}
