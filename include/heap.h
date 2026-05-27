#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

struct heap_chunk {
    size_t size;
    int is_free;
    struct heap_chunk *next;
};

void heap_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);

#endif