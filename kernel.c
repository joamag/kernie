#include "vga.h"
#include "serial.h"
#include "interrupts.h"
#include "shell.h"

__attribute__((section(".text.kernel_main"))) void kernel_main(void) {
    serial_init();
    vga_clear();

    interrupts_init();
    shell_init();

    /* idle loop */
    for (;;)
        __asm__ volatile ("hlt");
}
