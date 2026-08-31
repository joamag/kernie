/**
 * arch/riscv64/arch.c
 *
 * The architecture hooks for riscv64.
 *
 * There is no trap vector and no interrupt controller yet, so arch_init has
 * nothing to set up and input is polled from the idle loop instead of
 * arriving by interrupt. That is also why arch_ticks reports zero, there is no
 * timer running.
 *
 * Reboot and power off drive the SiFive test device the virt machine exposes.
 * The usual route on riscv64 is an SBI call, but the kernel boots with no
 * firmware beneath it, so there is nothing to make that call to.
 */

#include "kernel/arch.h"
#include "kernel/input.h"
#include "drivers/serial.h"

#define TEST_BASE  ((volatile uint32_t *)0x100000)
#define TEST_RESET 0x7777
#define TEST_PASS  0x5555

void arch_init(void) {
    // no trap vector yet, input is polled from the idle loop instead
}

void arch_idle(void) {
    int c = serial_getchar();
    if (c >= 0) {
        input_handle_char((char)c);
    }
}

void arch_reboot(void) {
    *TEST_BASE = TEST_RESET;
}

void arch_shutdown(void) {
    *TEST_BASE = TEST_PASS;
}

uint64_t arch_ticks(void) {
    // no timer yet, the machine timer is still to be wired up
    return 0;
}
