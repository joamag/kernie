/**
 * arch/x86_64/arch.c
 *
 * The architecture hooks for x86-64.
 *
 * Reboot pulses the keyboard controller reset line, which is the most widely
 * supported mechanism and needs no ACPI tables. Power off writes the ACPI
 * sleep state, trying three ports because the address differs between
 * emulators; only the first works under current QEMU and the others are kept
 * for older builds and for VirtualBox.
 */

#include "kernel/arch.h"
#include "arch/x86_64/interrupts.h"
#include "arch/x86_64/io.h"

extern volatile uint64_t tick_count;

void arch_init(void) {
    interrupts_init();
}

void arch_idle(void) {
    __asm__ volatile ("hlt");
}

void arch_reboot(void) {
    // pulse the keyboard controller reset line
    outb(0x64, 0xFE);
}

void arch_shutdown(void) {
    // enter ACPI sleep state 5, the port differs between emulators
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);
}

uint64_t arch_ticks(void) {
    return tick_count;
}
