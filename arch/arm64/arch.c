/**
 * arch/arm64/arch.c
 *
 * The architecture hooks for arm64.
 *
 * There is no vector table and no GIC yet, so arch_init has nothing to set up
 * and input is polled from the idle loop instead of arriving by interrupt.
 * That is also why arch_ticks reports zero, there is no timer running.
 *
 * Reboot and power off go through PSCI, which the virt machine exposes over
 * the hvc conduit when no secure world is present.
 */

#include "kernel/arch.h"
#include "kernel/input.h"
#include "drivers/serial.h"

void arch_init(void) {
    // no GIC yet, input is polled from the idle loop instead
}

void arch_idle(void) {
    int c = serial_getchar();
    if (c >= 0) {
        input_handle_char((char)c);
    }
}

// the virt machine exposes PSCI through hvc when there is no secure world
static void psci_call(uint32_t fn) {
    register uint64_t x0 __asm__("x0") = fn;
    __asm__ volatile ("hvc #0" : "+r"(x0) : : "memory");

    // neither SYSTEM_OFF nor SYSTEM_RESET returns when it succeeds, so
    // arriving here means the call was rejected and x0 carries the reason
    serial_print("PSCI call rejected, status ");
    serial_print_hex(x0);
    serial_print("\n");
}

void arch_reboot(void) {
    psci_call(0x84000009);
}

void arch_shutdown(void) {
    psci_call(0x84000008);
}

uint64_t arch_ticks(void) {
    // no timer yet, the generic timer is still to be wired up
    return 0;
}
