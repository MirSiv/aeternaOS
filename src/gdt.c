#include "gdt.h"

extern void klog(const char *str);

static struct {
    struct gdt_entry null_entry;
    struct gdt_entry code_entry;
    struct gdt_entry data_entry;
    struct gdt_tss_entry tss_entry;
} __attribute__((packed)) gdt;

static struct gdt_ptr gdt_p;
static struct tss_entry tss;
static uint8_t interrupt_stack[16384];

void gdt_set_gate(struct gdt_entry *entry, uint8_t access, uint8_t gran) {
    entry->limit_low = 0;
    entry->base_low = 0;
    entry->base_middle = 0;
    entry->access = access;
    entry->granularity = gran;
    entry->base_high = 0;
}

void gdt_init(void) {
    klog("[GDT] initialization of data structures...\n");

    gdt_set_gate(&gdt.null_entry, 0, 0);
    gdt_set_gate(&gdt.code_entry, 0x9A, 0x20);
    gdt_set_gate(&gdt.data_entry, 0x92, 0x00);

    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(tss) - 1;

    gdt.tss_entry.limit_low = tss_limit & 0xFFFF;
    gdt.tss_entry.base_low = tss_base & 0xFFFF;
    gdt.tss_entry.base_middle = (tss_base >> 16) & 0xFF;
    gdt.tss_entry.access = 0x89;
    gdt.tss_entry.granularity = (tss_limit >> 16) & 0x0F;
    gdt.tss_entry.base_high = (tss_base >> 24) & 0xFF;
    gdt.tss_entry.base_upper = (tss_base >> 32) & 0xFFFFFFFF;
    gdt.tss_entry.reserved = 0;

    tss.rsp0 = (uint64_t)&interrupt_stack[16384];
    tss.iomap_base = sizeof(tss);

    gdt_p.limit = sizeof(gdt) - 1;
    gdt_p.base = (uint64_t)&gdt;

    extern void gdt_flush(uint64_t);
    extern void tss_flush(void);
    
    klog("[GDT] lgdt...\n");
    gdt_flush((uint64_t)&gdt_p);
    klog("[GDT] segment register update completed...\n");

    klog("[TSS] loading the task register (ltr)...\n");
    tss_flush();
    klog("[TSS] safe interrupt stack enabled...\n");
}