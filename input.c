#include "input.h"
#include "vga.h"
#include "serial.h"

void input_handle_char(char c) {
    char str[2] = {c, 0};
    vga_print(str, VGA_WHITE_ON_BLACK);
    serial_putchar(c);
}
