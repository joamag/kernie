#include "vga.h"
#include "elf.h"

void kernel_main(void) {
    vga_clear();
    vga_print("=== Mini Kernel ===\n", VGA_GREEN_ON_BLACK);
    vga_print("VGA text mode: OK\n", VGA_WHITE_ON_BLACK);
    vga_print("ELF loader:    OK\n", VGA_WHITE_ON_BLACK);
    vga_print("\n", VGA_WHITE_ON_BLACK);
    vga_print("Kernel loaded at: ", VGA_WHITE_ON_BLACK);
    vga_print_hex(0x100000, VGA_RED_ON_BLACK);
    vga_print("\n", VGA_WHITE_ON_BLACK);
    vga_print("Hello from C in long mode!\n", VGA_GREEN_ON_BLACK);
}
