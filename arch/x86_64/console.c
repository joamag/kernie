#include "kernel/console.h"
#include "drivers/vga.h"
#include "drivers/serial.h"

/* x86-64 has both a VGA text buffer and a UART, so mirror everything to both */

void console_init(void) {
    serial_init();
}

void console_clear(void) {
    vga_clear();
}

void console_print(const char *str, uint8_t color) {
    vga_print(str, color);
    serial_print(str);
}

void console_print_hex(uint64_t val, uint8_t color) {
    vga_print_hex(val, color);
    serial_print_hex(val);
}
