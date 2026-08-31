/**
 * drivers/keyboard.h
 *
 * PS/2 keyboard translation, present on x86-64 only.
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_handle(uint8_t scancode);

#endif
