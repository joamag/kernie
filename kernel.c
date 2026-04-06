#include "vga.h"
#include "serial.h"
#include "elf.h"

__attribute__((section(".text.kernel_main"))) void kernel_main(void) {
    serial_init();

    vga_clear();
    vga_print("=== Mini Kernel ===\n", VGA_GREEN_ON_BLACK);
    serial_print("=== Mini Kernel ===\n");

    vga_print("VGA text mode: OK\n", VGA_WHITE_ON_BLACK);
    serial_print("VGA text mode: OK\n");

    vga_print("ELF loader:    OK\n", VGA_WHITE_ON_BLACK);
    serial_print("ELF loader:    OK\n");

    vga_print("\n", VGA_WHITE_ON_BLACK);
    serial_print("\n");

    vga_print("Kernel loaded at: ", VGA_WHITE_ON_BLACK);
    serial_print("Kernel loaded at: ");

    vga_print_hex(0x100000, VGA_RED_ON_BLACK);
    serial_print_hex(0x100000);

    vga_print("\n", VGA_WHITE_ON_BLACK);
    serial_print("\n");

    vga_print("Hello from C in long mode!\n", VGA_GREEN_ON_BLACK);
    serial_print("Hello from C in long mode!\n");
}
