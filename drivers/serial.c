#include "drivers/serial.h"

/* formatting shared by every UART, the driver underneath supplies the
   init, putchar and getchar primitives */

void serial_print(const char *str) {
    while (*str) {
        if (*str == '\n')
            serial_putchar('\r');
        serial_putchar(*str++);
    }
}

void serial_print_hex(uint64_t val) {
    const char *hex = "0123456789ABCDEF";
    char buf[19] = "0x0000000000000000";
    for (int i = 17; i >= 2; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    serial_print(buf);
}
