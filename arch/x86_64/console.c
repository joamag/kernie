/**
 * arch/x86_64/console.c
 *
 * Console for x86-64, mirroring output to both the VGA text buffer and the
 * UART so that the same session is visible on a screen and over a serial
 * line.
 */

#include "kernel/console.h"
#include "drivers/vga.h"
#include "drivers/serial.h"

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
