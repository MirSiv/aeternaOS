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

void klog_hex(uint64_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[18];
    int i = 0;
    
    if (value == 0) {
        klog("0x0");
        return;
    }
    
    // Build hex string in reverse
    buffer[i++] = 'x';
    buffer[i++] = '0';
    while (value > 0) {
        buffer[i++] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    // Print in correct order
    while (i > 0) {
        write_serial_char(buffer[--i]);
    }
}

void klog_dec(int64_t value) {
    char buffer[22];
    int i = 0;
    int negative = 0;
    
    if (value < 0) {
        negative = 1;
        value = -value;
    }
    
    if (value == 0) {
        klog("0");
        return;
    }
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    if (negative) {
        write_serial_char('-');
    }
    
    while (i > 0) {
        write_serial_char(buffer[--i]);
    }
}