#include "idt.h"
#include "serial.h"

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