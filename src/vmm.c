#include "vmm.h"
#include "pmm.h"
#include "serial.h"

static uint64_t *pml4_table;

void vmm_init(void) {
    klog("[VMM] старт init_vmm...\n");

    pml4_table = (uint64_t*)pmm_alloc_page();
    if (!pml4_table) {
        klog("[VMM] err: pmm did not provide a page for pml4\n");
    }
    
    for(int i = 0; i < 512; i++) {
        pml4_table[i] = 0;
    }
    klog("[VMM] pml4 root table created\n");

    klog("[VMM] mapping the first 16 megabytes (identity map)...\n");
    for (uint64_t addr = 0; addr < 16 * 1024 * 1024; addr += PAGE_SIZE) {
        vmm_map(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
    klog("[VMM] 16mb mapping completed successfully\n");

    klog("[VMM] loading new pml4 to cr3...\n");
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4_table));
    klog("[VMM] new pml4 page tables activated\n");
}

void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;

    uint64_t *curr_table = pml4_table;

    if (!(curr_table[pml4_idx] & PAGE_PRESENT)) {
        uint64_t* next = (uint64_t*)pmm_alloc_page();
        if ((uint64_t)next >= 8 * 1024 * 1024) {
            klog("[VMM] warning: pmm highlighted the page above the temporary mapping\n");
        }
        for(int i=0; i<512; i++) next[i] = 0;
        curr_table[pml4_idx] = (uint64_t)next | PAGE_PRESENT | PAGE_WRITABLE;
    }
    curr_table = (uint64_t*)(curr_table[pml4_idx] & ~0xFFF);

    if (!(curr_table[pdpt_idx] & PAGE_PRESENT)) {
        uint64_t* next = (uint64_t*)pmm_alloc_page();
        for(int i=0; i<512; i++) next[i] = 0;
        curr_table[pdpt_idx] = (uint64_t)next | PAGE_PRESENT | PAGE_WRITABLE;
    }
    curr_table = (uint64_t*)(curr_table[pdpt_idx] & ~0xFFF);
    if (!(curr_table[pd_idx] & PAGE_PRESENT)) {
        uint64_t* next = (uint64_t*)pmm_alloc_page();
        for(int i=0; i<512; i++) next[i] = 0;
        curr_table[pd_idx] = (uint64_t)next | PAGE_PRESENT | PAGE_WRITABLE;
    }
    curr_table = (uint64_t*)(curr_table[pd_idx] & ~0xFFF);
    
    curr_table[pt_idx] = (phys & ~0xFFF) | flags;
}