#include "drivers/serial.h"
#include "arch/x86_64/io.h"

#define SERIAL_COM1 0x3F8

void serial_init(void) {
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x80);
    outb(SERIAL_COM1 + 0, 0x01);
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x03);
    outb(SERIAL_COM1 + 2, 0xC7);
    outb(SERIAL_COM1 + 4, 0x0B);
}

void serial_putchar(char c) {
    while ((inb(SERIAL_COM1 + 5) & 0x20) == 0)
        ;
    outb(SERIAL_COM1, c);
}

int serial_getchar(void) {
    if ((inb(SERIAL_COM1 + 5) & 0x01) == 0)
        return -1;
    return inb(SERIAL_COM1);
}
