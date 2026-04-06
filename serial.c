#include "serial.h"
#include "io.h"

#define SERIAL_COM1 0x3F8

void serial_init(void) {
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x80);
    outb(SERIAL_COM1 + 0, 0x01);
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x03);
    outb(SERIAL_COM1 + 2, 0xC7);
    outb(SERIAL_COM1 + 4, 0x03);
}

void serial_putchar(char c) {
    while ((inb(SERIAL_COM1 + 5) & 0x20) == 0)
        ;
    outb(SERIAL_COM1, c);
}

void serial_print(const char *str) {
    while (*str) {
        if (*str == '\n')
            serial_putchar('\r');
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
