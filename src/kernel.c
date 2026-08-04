#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "serial.h"
#include "sched.h"
#include "syscall.h"

void test_thread1(void);
void test_thread2(void);

// Тест системных вызовов
void test_syscalls(void) {
    klog("\r\n[TEST] === SYSCALL TEST START ===\r\n");
    
    // Тест SYS_WRITE (syscall 2)
    klog("[TEST] Testing SYS_WRITE...\r\n");
    uint64_t syscall_num = 2;
    uint64_t fd = 1;
    uint64_t buf = 0x1000;  // адрес буфера (заглушка)
    uint64_t count = 13;
    
    __asm__ volatile (
        "mov %0, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "mov %3, %%rdx\n\t"
        "syscall\n\t"
        :
        : "r"(syscall_num), "r"(fd), "r"(buf), "r"(count)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r11"
    );
    
    // Тест SYS_READ (syscall 1)
    klog("[TEST] Testing SYS_READ...\r\n");
    syscall_num = 1;
    fd = 0;
    buf = 0x2000;
    count = 10;
    
    __asm__ volatile (
        "mov %0, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "mov %3, %%rdx\n\t"
        "syscall\n\t"
        :
        : "r"(syscall_num), "r"(fd), "r"(buf), "r"(count)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r11"
    );
    
    // Тест SYS_OPEN (syscall 3)
    klog("[TEST] Testing SYS_OPEN...\r\n");
    syscall_num = 3;
    fd = 0x5000;  // путь к файлу (заглушка)
    uint64_t flags = 0x2;  // O_RDWR
    
    __asm__ volatile (
        "mov %0, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "syscall\n\t"
        :
        : "r"(syscall_num), "r"(fd), "r"(flags)
        : "rax", "rdi", "rsi", "rcx", "r11"
    );
    
    // Тест SYS_CLOSE (syscall 4)
    klog("[TEST] Testing SYS_CLOSE...\r\n");
    syscall_num = 4;
    fd = 3;
    
    __asm__ volatile (
        "mov %0, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "syscall\n\t"
        :
        : "r"(syscall_num), "r"(fd)
        : "rax", "rdi", "rcx", "r11"
    );
    
    // Тест неизвестного syscall
    klog("[TEST] Testing unknown syscall (999)...\r\n");
    syscall_num = 999;
    
    __asm__ volatile (
        "mov %0, %%rax\n\t"
        "syscall\n\t"
        :
        : "r"(syscall_num)
        : "rax", "rcx", "r11"
    );
    
    klog("[TEST] === SYSCALL TEST END ===\r\n");
}

void kernel_main(uint64_t mb_addr) {
    gdt_init();
    idt_init();
    
    pmm_init(mb_addr);
    vmm_init();
    
    heap_init();
    
    // Инициализация системных вызовов
    syscall_init();
    
    vga_set_color(0, 15);
    vga_clear(); 
    
    vga_set_color(0, 2);
    kprint("aeternaOS booted succesfully\n");

    vga_set_color(15, 0);
    kprint("everything works, ig\n");

    klog("[kernel] all subsystems initialized, spinning up threads...\n");

    init_scheduler();
    create_kernel_thread(test_thread1);
    create_kernel_thread(test_thread2);

    asm volatile("sti");

    while(1) {
        asm volatile("hlt");
    }
}

void test_thread1(void) {
    while (1) {
        klog("1");
        for (volatile int i = 0; i < 1000000; i++); 
    }
}

void test_thread2(void) {
    while (1) {
        klog("2");
        for (volatile int i = 0; i < 1000000; i++);
    }
}