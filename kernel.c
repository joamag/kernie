#include "vga.h"
#include "serial.h"
#include "interrupts.h"
#include "shell.h"
#include "input.h"
#include "mem.h"

/* placed by kernel.ld, the flat binary carries no .bss so it holds whatever
   was in memory until we clear it */
extern uint8_t __bss_start[], __bss_end[];

__attribute__((section(".text.kernel_main"))) void kernel_main(void) {
    memset(__bss_start, 0, __bss_end - __bss_start);

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
