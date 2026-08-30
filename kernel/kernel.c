/**
 * kernel/kernel.c
 *
 * Kernel entry point, common to every architecture.
 *
 * The first thing it does is clear .bss, which is not optional. The section is
 * NOBITS, so it is absent from the flat binary the x86-64 bootloader copies
 * and from the ELF QEMU loads on arm64, and it therefore holds whatever was in
 * memory until this runs. It works under emulation only because QEMU hands out
 * zeroed RAM.
 *
 * The shell is brought up before interrupts are enabled, so that a keystroke
 * already pending at boot cannot drive the shell while the banner is still
 * being written.
 *
 * The idle loop drains the input queue and then hands control to the
 * architecture, rather than halting here, because an architecture without
 * interrupts has to poll and would never wake from a halt.
 */

#include "kernel/console.h"
#include "kernel/arch.h"
#include "kernel/shell.h"
#include "kernel/input.h"
#include "lib/mem.h"

// placed by the linker script, the loaded image carries no .bss so it holds
// whatever was in memory until we clear it
extern uint8_t __bss_start[], __bss_end[];

__attribute__((section(".text.kernel_main"))) void kernel_main(void) {
    memset(__bss_start, 0, (uintptr_t)__bss_end - (uintptr_t)__bss_start);

    console_init();
    console_clear();

    // bring the shell up first, so the banner cannot race an early IRQ
    shell_init();
    arch_init();

    // idle loop
    for (;;) {
        input_poll();
        arch_idle();
    }
}
