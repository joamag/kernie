/**
 * drivers/serial.h
 *
 * The UART interface, split between formatting that every port shares and the
 * init, putchar and getchar primitives each port provides for itself.
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

/**
 * Prepares the UART. Provided by the driver for the port in use.
 */
void serial_init(void);

/**
 * Writes one byte, blocking until the transmitter has room.
 */
void serial_putchar(char c);

/**
 * Reads one pending byte, or returns -1 when nothing has arrived.
 */
int serial_getchar(void);

/**
 * Writes a string, translating a line feed into a carriage return pair so a
 * terminal does not staircase.
 */
void serial_print(const char *str);

/**
 * Writes a value as fixed width hexadecimal, zero padded and 0x prefixed.
 */
void serial_print_hex(uint64_t val);

#endif
