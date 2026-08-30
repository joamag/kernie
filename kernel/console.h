#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

/* colour attributes follow the VGA text encoding, architectures without a
   text buffer simply ignore them */
#define CONSOLE_WHITE   0x0F
#define CONSOLE_GREEN   0x0A
#define CONSOLE_RED     0x04
#define CONSOLE_GRAY    0x08
#define CONSOLE_BLUE    0x09
#define CONSOLE_CYAN    0x0B
#define CONSOLE_MAGENTA 0x0D
#define CONSOLE_YELLOW  0x0E

void console_init(void);
void console_clear(void);
void console_print(const char *str, uint8_t color);
void console_print_hex(uint64_t val, uint8_t color);

#endif
