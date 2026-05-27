#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "serial.h"
#include "sched.h"

void test_thread1(void);
void test_thread2(void);

void kernel_main(uint64_t mb_addr) {
    gdt_init();
    idt_init();
    
    pmm_init(mb_addr);
    vmm_init();
    
    heap_init();
    
    init_scheduler();
    create_thread(test_thread1);
    create_thread(test_thread2);

    vga_set_color(0, 15);
    vga_clear(); 
    
    vga_set_color(0, 2);
    kprint("aeternaOS booted succesfully\n");

    vga_set_color(15, 0);
    kprint("everything works, ig\n");

    klog("[kernel] all subsystems initialized, spinning up threads...\n");

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