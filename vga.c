#include "vga.h"

#define VGA_BUFFER 0xB8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static uint16_t *vga = (uint16_t *)VGA_BUFFER;
static int cursor = 0;

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (VGA_WHITE_ON_BLACK << 8) | ' ';
    cursor = 0;
}

void vga_putchar(char c, uint8_t color) {
    if (c == '\n') {
        cursor += VGA_WIDTH - (cursor % VGA_WIDTH);
    } else {
        vga[cursor] = (color << 8) | c;
        cursor++;
    }

    if (cursor >= VGA_WIDTH * VGA_HEIGHT) {
        for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++)
            vga[i] = vga[i + VGA_WIDTH];
        for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++)
            vga[i] = (VGA_WHITE_ON_BLACK << 8) | ' ';
        cursor -= VGA_WIDTH;
    }
}

void vga_print(const char *str, uint8_t color) {
    while (*str)
        vga_putchar(*str++, color);
}

void vga_print_hex(uint64_t val, uint8_t color) {
    char buf[19] = "0x0000000000000000";
    const char *hex = "0123456789ABCDEF";
    for (int i = 17; i >= 2; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    vga_print(buf, color);
}
