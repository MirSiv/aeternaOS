#include <stdint.h>
#include "vga.h"
#include "gdt.h" 
#include "idt.h"
#include "pmm.h"
#include "vmm.h"
#include "serial.h"

void kernel_main(uint64_t mb_addr) {
    // vga help below
    // vga_set_color(TEXT, BG)
    vga_set_color(0, 15);
    vga_clear();
    vga_set_color(15, 0);
    kprint("everything is ok ig\n");
    vga_set_color(2, 0);
    kprint("aeternaOS booted succesfully\n");

    gdt_init();
    idt_init();
    pmm_init(mb_addr);
    vmm_init();
    __asm__ volatile ("sti");
    klog("[kernel] interrupts enabled, system ready\n");

    while(1) {
        __asm__ volatile("hlt");
    }
}