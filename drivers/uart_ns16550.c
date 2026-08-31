/**
 * drivers/uart_ns16550.c
 *
 * The NS16550A UART on the QEMU riscv64 virt machine.
 *
 * The register layout is the same one the x86-64 COM1 driver drives, but
 * riscv64 has no port space, so the registers are reached as bytes in the
 * memory map instead. That difference is the whole reason this is a separate
 * driver rather than a shared one.
 *
 * The divisor is left alone. Setting it would mean knowing the input clock,
 * which is only discoverable from the device tree the kernel does not parse
 * yet, and QEMU leaves the port usable without it.
 */

#include "drivers/serial.h"

#define NS16550_BASE 0x10000000
#define NS16550_RBR  ((volatile uint8_t *)(NS16550_BASE + 0))
#define NS16550_FCR  ((volatile uint8_t *)(NS16550_BASE + 2))
#define NS16550_LCR  ((volatile uint8_t *)(NS16550_BASE + 3))
#define NS16550_LSR  ((volatile uint8_t *)(NS16550_BASE + 5))

#define LSR_DR   (1 << 0)
#define LSR_THRE (1 << 5)

void serial_init(void) {
    *NS16550_LCR = 0x03;
    *NS16550_FCR = 0xC7;
}

void serial_putchar(char c) {
    while ((*NS16550_LSR & LSR_THRE) == 0) {
        // wait for the holding register to drain
    }
    *NS16550_RBR = (uint8_t)c;
}

int serial_getchar(void) {
    if ((*NS16550_LSR & LSR_DR) == 0) {
        return -1;
    }
    return (int)*NS16550_RBR;
}
