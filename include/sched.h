#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) context_t;

typedef struct thread {
    int id;                     
    uint64_t kstack;            
    struct thread* next;        
    int state;                  
} thread_t;

void init_scheduler(void);
thread_t* create_thread(void (*entry_point)(void));
uint64_t schedule(uint64_t current_rsp);

#endif