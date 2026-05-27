#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER     (1 << 2)

void vmm_init(void);
void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags);

#endif