#include "pmm.h"
#include "serial.h"

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot_mmap_tag {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} __attribute__((packed));

struct multiboot_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

static uint8_t *bitmap;
static uint64_t bitmap_size = 0;
static uint64_t total_pages = 0;

#define SET_BIT(page)   (bitmap[(page) / 8] |= (1 << ((page) % 8)))
#define CLEAR_BIT(page) (bitmap[(page) / 8] &= ~(1 << ((page) % 8)))
#define TEST_BIT(page)  (bitmap[(page) / 8] & (1 << ((page) % 8)))

void pmm_init(uint64_t mb_addr) {
    klog("[PMM] parsing multiboot2 structures...\n");

    char hex_buf[19];
    char hex_chars[] = "0123456789ABCDEF";
    hex_buf[0] = '0';
    hex_buf[1] = 'x';
    for (int i = 0; i < 16; i++) {
        hex_buf[2 + i] = hex_chars[(mb_addr >> (60 - i * 4)) & 0xF];
    }
    hex_buf[18] = '\0';

    klog("  -> mb_addr came as: ");
    klog(hex_buf);
    klog("\n");

    uint32_t total_size = *(volatile uint32_t*)mb_addr;

    for (int i = 0; i < 8; i++) {
        hex_buf[2 + i] = hex_chars[(total_size >> (28 - i * 4)) & 0xF];
    }
    hex_buf[10] = '\0';

    klog("  -> read total_size: ");
    klog(hex_buf);
    klog("\n");

    if (total_size < 8 || total_size > 1024 * 1024) {
        klog("[PMM] something crazy is happening. look up\n");
        while(1) __asm__ volatile("hlt");
    }

    uint8_t *ptr = (uint8_t*)(mb_addr + 8);
    uint64_t mem_upper = 0;

    while (ptr < (uint8_t*)(mb_addr + total_size)) {
        struct multiboot_tag *tag = (struct multiboot_tag*)ptr;

        if (tag->type == 0 && tag->size == 8) break;

        if (tag->type == 6) {
            struct multiboot_mmap_tag *mmap = (struct multiboot_mmap_tag*)ptr;
            uint8_t *entry_ptr = ptr + sizeof(struct multiboot_mmap_tag);

            while (entry_ptr < ptr + mmap->size) {
                struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry*)entry_ptr;

                if (entry->type == 1) {
                    if (entry->base_addr + entry->length > mem_upper) {
                        mem_upper = entry->base_addr + entry->length;
                    }
                }
                entry_ptr += mmap->entry_size;
            }
        }
        ptr += ((tag->size + 7) & ~7);
    }

    if (mem_upper == 0) {
        klog("[PMM] err: unable to determine ram size, default will be 128MB\n");
        mem_upper = 128 * 1024 * 1024;
    }

    total_pages = mem_upper / PAGE_SIZE;
    bitmap_size = total_pages / 8;

    bitmap = (uint8_t*)0x400000;

    for (uint64_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    ptr = (uint8_t*)(mb_addr + 8);
    while (ptr < (uint8_t*)(mb_addr + total_size)) {
        struct multiboot_tag *tag = (struct multiboot_tag*)ptr;
        if (tag->type == 0 && tag->size == 8) break;

        if (tag->type == 6) {
            struct multiboot_mmap_tag *mmap = (struct multiboot_mmap_tag*)ptr;
            uint8_t *entry_ptr = ptr + sizeof(struct multiboot_mmap_tag);

            while (entry_ptr < ptr + mmap->size) {
                struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry*)entry_ptr;

                if (entry->type == 1) {
                    uint64_t start_page = entry->base_addr / PAGE_SIZE;
                    uint64_t end_page = (entry->base_addr + entry->length) / PAGE_SIZE;
                    for (uint64_t p = start_page; p < end_page; p++) {
                        CLEAR_BIT(p);
                    }
                }
                entry_ptr += mmap->entry_size;
            }
        }
        ptr += ((tag->size + 7) & ~7);
    }

    uint64_t reserved_pages = (0x400000 + bitmap_size) / PAGE_SIZE + 1;
    for (uint64_t p = 0; p < reserved_pages; p++) {
        SET_BIT(p);
    }

    klog("[PMM] physical memory manager was initialized successfully\n");
}

void *pmm_alloc_page(void) {
    for (uint64_t p = 0; p < total_pages; p++) {
        if (!TEST_BIT(p)) {
            SET_BIT(p);
            return (void*)(p * PAGE_SIZE);
        }
    }
    klog("[PMM] err: out of memory!\n");
    return NULL;
}

void pmm_free_page(void *addr) {
    uint64_t page = (uint64_t)addr / PAGE_SIZE;
    CLEAR_BIT(page);
}