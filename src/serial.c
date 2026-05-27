#include <stdint.h>

#define PORT 0x3f8

// hi there

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void init_serial(void) {
    outb(PORT + 1, 0x00);    
    outb(PORT + 3, 0x80);    
    outb(PORT + 0, 0x03);    
    outb(PORT + 1, 0x00);    
    outb(PORT + 3, 0x03);    
    outb(PORT + 2, 0xC7);    
    outb(PORT + 4, 0x0B);    
}

int is_transmit_empty(void) {
    return inb(PORT + 5) & 0x20;
}

void write_serial_char(char a) {
    while (is_transmit_empty() == 0);
    outb(PORT, a);
}

void klog(const char *str) {
    while (*str) {
        write_serial_char(*str++);
    }
}