#include "vga.h"
#include "serial.h"
#include "interrupts.h"
#include "shell.h"
#include "input.h"

__attribute__((section(".text.kernel_main"))) void kernel_main(void) {
    serial_init();
    vga_clear();

    /* bring the shell up first, so the banner cannot race an early IRQ */
    shell_init();
    interrupts_init();

    /* idle loop */
    for (;;) {
        input_poll();
        __asm__ volatile ("hlt");
    }
}
