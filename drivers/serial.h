#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

void serial_init(void);
void serial_putchar(char c);
int serial_getchar(void);
void serial_print(const char *str);
void serial_print_hex(uint64_t val);

#endif
