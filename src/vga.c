#include "vga.h"
#include <stdint.h>

static uint16_t *vga_buffer = (uint16_t*)0xb8000;
static int vga_x = 0;
static int vga_y = 0;
static uint8_t current_color = 0x0F;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

void vga_set_color(enum vga_color fg, enum vga_color bg) {
    current_color = vga_entry_color(fg, bg);
}

void vga_clear(void) {
    uint16_t blank = (uint16_t)' ' | ((uint16_t)current_color << 8);
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = blank;
    }
    vga_x = 0;
    vga_y = 0;
}

void kprint(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            vga_x = 0;
            vga_y++;
        } else {
            vga_buffer[vga_y * 80 + vga_x] = (uint16_t)str[i] | ((uint16_t)current_color << 8);
            vga_x++;
        }

        if (vga_x >= 80) {
            vga_x = 0;
            vga_y++;
        }
        
        if (vga_y >= 25) {
            vga_clear();
        }
    }
}