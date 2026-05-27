#include "heap.h"
#include "vmm.h"
#include "pmm.h"
#include "serial.h"

#define HEAP_START 0x20000000
#define HEAP_INITIAL_PAGES 4

static struct heap_chunk *head = NULL;
static uint64_t heap_current_end = HEAP_START;

static int expand_heap(size_t bytes) {
    size_t pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    for (size_t i = 0; i < pages; i++) {
        void *phys = pmm_alloc_page();
        if (!phys) {
            klog("[HEAP] err: no physical memory to expand heap!\n");
            return 0;
        }
        vmm_map(heap_current_end, (uint64_t)phys, PAGE_PRESENT | PAGE_WRITABLE);
        heap_current_end += PAGE_SIZE;
    }
    return 1;
}

void heap_init(void) {
    klog("[HEAP] kernel heap initialization...\n");

    expand_heap(HEAP_INITIAL_PAGES * PAGE_SIZE);

    head = (struct heap_chunk*)HEAP_START;
    head->size = (HEAP_INITIAL_PAGES * PAGE_SIZE) - sizeof(struct heap_chunk);
    head->is_free = 1;
    head->next = NULL;

    klog("[HEAP] kernel heap started successfully\n");
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    size = (size + 7) & ~7;

    struct heap_chunk *curr = head;
    struct heap_chunk *best_fit = NULL;

    while (curr) {
        if (curr->is_free && curr->size >= size) {
            if (!best_fit || curr->size < best_fit->size) {
                best_fit = curr;
                if (best_fit->size == size) break; 
            }
        }
        curr = curr->next;
    }

    if (!best_fit) {
        size_t required_space = size + sizeof(struct heap_chunk);
        struct heap_chunk *new_chunk = (struct heap_chunk*)heap_current_end;
        
        if (!expand_heap(required_space)) {
            klog("[HEAP] err: out of Memory in kmalloc!\n");
            return NULL;
        }

        new_chunk->size = (heap_current_end - (uint64_t)new_chunk) - sizeof(struct heap_chunk);
        new_chunk->is_free = 1;
        new_chunk->next = NULL;

        curr = head;
        while (curr->next) curr = curr->next;
        curr->next = new_chunk;

        best_fit = new_chunk;
    }

    if (best_fit->size >= size + sizeof(struct heap_chunk) + 8) {
        struct heap_chunk *new_chunk = (struct heap_chunk*)((uint8_t*)best_fit + sizeof(struct heap_chunk) + size);
        
        new_chunk->size = best_fit->size - size - sizeof(struct heap_chunk);
        new_chunk->is_free = 1;
        new_chunk->next = best_fit->next;

        best_fit->size = size;
        best_fit->next = new_chunk;
    }

    best_fit->is_free = 0;

    return (void*)((uint8_t*)best_fit + sizeof(struct heap_chunk));
}

void kfree(void *ptr) {
    if (!ptr) return;

    struct heap_chunk *chunk = (struct heap_chunk*)((uint8_t*)ptr - sizeof(struct heap_chunk));
    chunk->is_free = 1;

    struct heap_chunk *curr = head;
    while (curr) {
        if (curr->is_free && curr->next && curr->next->is_free) {
            curr->size += sizeof(struct heap_chunk) + curr->next->size;
            curr->next = curr->next->next;
            continue; 
        }
        curr = curr->next;
    }
}