/**
 * arch/riscv64/console.c
 *
 * Console for riscv64.
 *
 * The virt machine has no text buffer, so the UART is the whole console and
 * the colour attribute is discarded rather than mapped to escape sequences,
 * which would assume a terminal on the other end. Clearing the screen is the
 * one exception, since there is no other way to express it.
 */

#include "kernel/console.h"
#include "drivers/serial.h"

void console_init(void) {
    serial_init();
}

void console_clear(void) {
    serial_print("\033[2J\033[H");
}

void console_print(const char *str, uint8_t color) {
    (void)color;
    serial_print(str);
}

void console_print_hex(uint64_t val, uint8_t color) {
    (void)color;
    serial_print_hex(val);
}
