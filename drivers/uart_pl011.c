/**
 * drivers/uart_pl011.c
 *
 * The PL011 UART on the QEMU virt machine, reached through memory mapped
 * registers rather than a port space, which arm64 does not have.
 *
 * No initialisation is performed. The firmware leaves the UART enabled at a
 * usable baud rate, and programming the divisor would mean knowing the clock,
 * which is only discoverable from the device tree the kernel does not parse
 * yet.
 */

#include "drivers/serial.h"

#define PL011_BASE 0x09000000
#define PL011_DR   ((volatile uint32_t *)(PL011_BASE + 0x00))
#define PL011_FR   ((volatile uint32_t *)(PL011_BASE + 0x18))

#define FR_RXFE (1 << 4)
#define FR_TXFF (1 << 5)

void serial_init(void) {
    // the firmware already leaves the UART enabled at a usable baud rate
}

void serial_putchar(char c) {
    while (*PL011_FR & FR_TXFF)
        ;
    *PL011_DR = (uint32_t)c;
}

int serial_getchar(void) {
    if (*PL011_FR & FR_RXFE)
        return -1;
    return (int)(*PL011_DR & 0xFF);
}
