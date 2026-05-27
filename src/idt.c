#include "idt.h"
#include "serial.h"
#include "io.h"
#include "keyboard.h"

static struct idt_entry idt[256];
static struct idt_ptr idt_p;

static const char *exception_messages[] = {
    "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
    "Into Detected Overflow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",
    "Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
    "Stack Fault", "General Protection Fault", "Page Fault", "Unknown Interrupt",
    "Coprocessor Fault", "Alignment Check", "Machine Check", "SIMD Floating-Point",
    "Virtualization", "Control Protection", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Security Exception", "Reserved"
};

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].isr_low = (uint16_t)(base & 0xFFFF);
    idt[num].kernel_cs = sel;
    idt[num].ist = 0;
    idt[num].attributes = flags;
    idt[num].isr_mid = (uint16_t)((base >> 16) & 0xFFFF);
    idt[num].isr_high = (uint32_t)((base >> 32) & 0xFFFFFFFF);
    idt[num].reserved = 0;
}

extern void *isr_stub_table[];
extern void irq0();
extern void irq1(); 
void pic_init(void) {
    klog("[PIC] remapping PIC to vectors 32-47...\n");
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);
    klog("[PIC] remap completed\n");
}
void timer_init(void) {
    klog("[PIT] setting timer frequency to 100hz...\n");
    uint32_t divisor = 1193182 / 100;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void idt_init(void) {
    klog("[IDT] start init_idt...\n");

    klog("[IDT] clearing all 256 descriptors...\n");
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    klog("[IDT] clear completed\n");

    klog("[IDT] filling the first 32 exceptions...\n");
    for (int i = 0; i < 32; i++) {
        uint64_t hook_address = (uint64_t)isr_stub_table[i];
        if (hook_address == 0) {
            klog("[IDT] err: the stub address = 0\n");
        }
        idt_set_gate(i, hook_address, 0x08, 0x8E);
    }
    klog("[IDT] 32 exceptions filled successfully\n");

    klog("[IDT] filling hardware IRQs...\n");
    idt_set_gate(32, (uint64_t)irq0, 0x08, 0x8E); 
    idt_set_gate(33, (uint64_t)irq1, 0x08, 0x8E); 

    pic_init();
    timer_init();

    idt_p.limit = sizeof(idt) - 1;
    idt_p.base = (uint64_t)&idt;

    klog("[IDT] calling asm lidt...\n");
    __asm__ volatile ("lidt %0" : : "m"(idt_p));
    klog("[IDT] asm lidt worked successfully\n");
}

void exception_handler(struct interrupt_frame *frame) {
    klog("\n---### we`ve got an exception :p ###---\n");
    if (frame->int_no < 32) {
        klog("today`s exception is: ");
        klog(exception_messages[frame->int_no]);
        klog("\n");
    }
    
    klog("[KERNEL] system stopped\n");
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

static uint64_t timer_ticks = 0;

void timer_handler(void) {
    timer_ticks++;
    if (timer_ticks % 100 == 0) {
        klog("[TIMER] 1 sec passed\n");
    }
    outb(0x20, 0x20);
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    keyboard_handle_scan(scancode);
    outb(0x20, 0x20);
}