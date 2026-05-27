#include "sched.h"
#include "serial.h"
#include "heap.h"
#define MAX_THREADS 16
#define STACK_SIZE 4096

static thread_t thread_table[MAX_THREADS];
static uint8_t thread_stacks[MAX_THREADS][STACK_SIZE];
static int thread_count = 0;

static thread_t* current_thread = 0;
static thread_t* idle_thread = 0;
void idle_task(void) {
    while (1) {
        asm volatile("hlt");
    }
}

void init_scheduler(void) {
    idle_thread = create_thread(idle_task);
    current_thread = idle_thread;
    current_thread->state = 1;
}

thread_t* create_thread(void (*entry_point)(void)) {
    if (thread_count >= MAX_THREADS) {
        klog("[SCHED] err: max threads reached\n");
        return 0;
    }

    thread_t* t = &thread_table[thread_count];
    t->id = thread_count;
    t->state = 0;

    uint64_t stack_top = (uint64_t)&thread_stacks[thread_count][STACK_SIZE];
    stack_top &= ~0xF;
    stack_top -= sizeof(context_t);
    context_t* ctx = (context_t*)stack_top;

    ctx->rflags = 0x202;
    ctx->cs = 0x08;
    ctx->ss = 0x10;
    ctx->rip = (uint64_t)entry_point;
    ctx->rsp = stack_top + sizeof(context_t) - 8;

    t->kstack = stack_top;
    if (thread_count > 0) {
        thread_table[thread_count - 1].next = t;
        t->next = &thread_table[0];
    } else {
        t->next = t;
    }

    thread_count++;
    return t;
}
uint64_t schedule(uint64_t current_rsp) {
    if (!current_thread) return current_rsp;

    current_thread->kstack = current_rsp;
    current_thread->state = 0;

    thread_t* next_thread = current_thread->next;

    if (!next_thread) {
        next_thread = idle_thread;
    }

    current_thread = next_thread;
    current_thread->state = 1;

    return current_thread->kstack;
}