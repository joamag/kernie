#include "vga.h"
#include "serial.h"
#include "elf.h"
#include "interrupts.h"

__attribute__((section(".text.kernel_main"))) void kernel_main(void) {
    serial_init();
    vga_clear();

    vga_print("=== Kernie the Kernel ===\n", VGA_GREEN_ON_BLACK);
    serial_print("=== Kernie the Kernel ===\n");

    interrupts_init();

    vga_print("Interrupts:    ON\n", VGA_GREEN_ON_BLACK);
    serial_print("Interrupts:    ON\n");

    /* idle loop */
    for (;;)
        __asm__ volatile ("hlt");
}
