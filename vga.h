#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_WHITE_ON_BLACK 0x0F
#define VGA_GREEN_ON_BLACK 0x0A
#define VGA_RED_ON_BLACK   0x04

void vga_clear(void);
void vga_putchar(char c, uint8_t color);
void vga_print(const char *str, uint8_t color);
void vga_print_hex(uint64_t val, uint8_t color);

#endif
