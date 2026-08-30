/**
 * drivers/serial.c
 *
 * UART output formatting, shared by every port.
 *
 * Only the three hardware primitives differ between a 16550 behind x86-64 port
 * I/O and a PL011 behind memory mapped registers, so the newline translation
 * and the hexadecimal printing live here once rather than in each driver.
 *
 * serial_print translates a line feed into a carriage return pair, because a
 * serial terminal will otherwise leave the cursor in the column it was in and
 * produce a staircase.
 */

#include "drivers/serial.h"

void serial_print(const char *str) {
    while (*str) {
        if (*str == '\n') {
            serial_putchar('\r');
        }
        serial_putchar(*str++);
    }
}

void serial_print_hex(uint64_t val) {
    const char *hex = "0123456789ABCDEF";
    char buf[19] = "0x0000000000000000";
    for (int i = 17; i >= 2; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    serial_print(buf);
}
