/**
 * drivers/uart_16550.c
 *
 * The 16550 UART on COM1, reached through x86-64 port I/O.
 *
 * The initialisation sequence sets the divisor for 115200 baud, eight data
 * bits with no parity, and enables the FIFO. The modem control register is
 * written 0x0B rather than 0x03 because OUT2, the third bit, gates the
 * interrupt line through to the PIC on PC compatible hardware. Without it the
 * UART cannot raise IRQ 4 no matter what the interrupt enable register says,
 * and the fault is invisible under emulation because QEMU does not model the
 * gate.
 */

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
    while ((inb(SERIAL_COM1 + 5) & 0x20) == 0) {
        // wait for the holding register to drain
    }
    outb(SERIAL_COM1, c);
}

int serial_getchar(void) {
    if ((inb(SERIAL_COM1 + 5) & 0x01) == 0) {
        return -1;
    }
    return inb(SERIAL_COM1);
}
