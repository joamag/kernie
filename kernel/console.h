/**
 * kernel/console.h
 *
 * The console contract every architecture implements.
 *
 * Shared code writes through this rather than reaching for a specific device,
 * which is what lets kernel/ compile unchanged for a target that has no text
 * buffer. The colour attributes follow the VGA text encoding because that is
 * the only backend where they mean anything, and an architecture without one
 * discards them rather than translating to escape sequences.
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

#define CONSOLE_WHITE   0x0F
#define CONSOLE_GREEN   0x0A
#define CONSOLE_RED     0x04
#define CONSOLE_GRAY    0x08
#define CONSOLE_BLUE    0x09
#define CONSOLE_CYAN    0x0B
#define CONSOLE_MAGENTA 0x0D
#define CONSOLE_YELLOW  0x0E

/**
 * Prepares every device backing the console.
 */
void console_init(void);

/**
 * Clears the console and returns the cursor to the origin.
 */
void console_clear(void);

/**
 * Writes a string in the given colour, which a backend without a text buffer
 * is free to ignore.
 */
void console_print(const char *str, uint8_t color);

/**
 * Writes a value as fixed width hexadecimal, zero padded to sixteen digits
 * and prefixed with 0x, which is the only numeric formatting available until
 * there is a proper printer.
 */
void console_print_hex(uint64_t val, uint8_t color);

#endif
