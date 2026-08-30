#include "kernel/console.h"
#include "drivers/serial.h"

/* the virt machine has no text buffer, so the console is the UART alone and
   the colour attribute is discarded */

void console_init(void) {
    serial_init();
}

void console_clear(void) {
    serial_print("\033[2J\033[H");
}

void console_print(const char *str, uint8_t color) {
    (void)color;
    serial_print(str);
}

void console_print_hex(uint64_t val, uint8_t color) {
    (void)color;
    serial_print_hex(val);
}
