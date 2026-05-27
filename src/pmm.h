#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

void pmm_init(uint64_t mb_addr);
void *pmm_alloc_page(void);
void pmm_free_page(void *addr);

#endif