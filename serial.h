#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define SERIAL_COM1 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void) {
    outb(SERIAL_COM1 + 1, 0x00);  // disable interrupts
    outb(SERIAL_COM1 + 3, 0x80);  // enable DLAB
    outb(SERIAL_COM1 + 0, 0x01);  // baud 115200 (divisor 1)
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x03);  // 8N1
    outb(SERIAL_COM1 + 2, 0xC7);  // enable FIFO
    outb(SERIAL_COM1 + 4, 0x03);  // RTS/DSR set
}

void serial_putchar(char c) {
    while ((inb(SERIAL_COM1 + 5) & 0x20) == 0)
        ;  // wait for transmit buffer empty
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

#endif
