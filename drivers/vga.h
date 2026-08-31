/**
 * drivers/vga.h
 *
 * VGA text mode output, present on x86-64 only.
 */

#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_WHITE_ON_BLACK 0x0F
#define VGA_GREEN_ON_BLACK 0x0A
#define VGA_RED_ON_BLACK   0x04
#define VGA_GRAY_ON_BLACK  0x08
#define VGA_BLUE_ON_BLACK  0x09
#define VGA_CYAN_ON_BLACK  0x0B
#define VGA_MAGENTA_ON_BLACK 0x0D
#define VGA_YELLOW_ON_BLACK 0x0E

void vga_clear(void);
void vga_putchar(char c, uint8_t color);
void vga_print(const char *str, uint8_t color);
void vga_print_hex(uint64_t val, uint8_t color);

#endif
