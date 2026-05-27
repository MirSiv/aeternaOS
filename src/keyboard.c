#include "keyboard.h"
#include "serial.h"

static const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t',
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, /* Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, /* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, /* Right shift */
  '*',
    0, /* Alt */
  ' ', /* Space */
    0, /* Caps lock */
    0,  0,   0,   0,   0,   0,   0,   0,   0,   0, /* F1-F10 */
    0, /* Num lock */
    0, /* Scrl lock */
    0, /* Hom */
    0, /* Up Arr */
    0, /* Pg Up */
  '-',
    0, /* Left Arrow */
    0,
    0, /* Right Arrow */
  '+',
    0, /* End */
    0, /* Down Arrow */
    0, /* Page Down */
    0, /* Insert */
    0, /* Delete */
    0,   0,   0,
    0, /* F11 */
    0, /* F12 */
};

void keyboard_handle_scan(uint8_t scancode) {
    if (scancode & 0x80) {
        return;
    }

    if (scancode < 128) {
        char ascii = kbd_us[scancode];
        
        if (ascii != 0) {
            char str[2] = {ascii, 0};
            klog(str); 
        }
    }
}