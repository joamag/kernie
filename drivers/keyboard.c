/**
 * drivers/keyboard.c
 *
 * PS/2 keyboard, translating scan code set 1 into ASCII.
 *
 * The two tables are indexed directly by scan code, which trades a little
 * space for removing any search from the interrupt path. Entries that have no
 * ASCII meaning are left zero and dropped by the caller.
 *
 * Left and right shift are tracked in separate flags rather than one shared
 * counter, so that releasing either key while the other is still held does not
 * cancel the shifted state.
 *
 * Caps lock, control and the 0xE0 prefixed extended keys are not handled
 * yet.
 */

#include "drivers/keyboard.h"
#include "kernel/input.h"

// US QWERTY scancode set 1 -> ASCII (lowercase)
static const char scancode_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ',
};

// shifted variants
static const char scancode_map_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' ',
};

static int shift_left = 0;
static int shift_right = 0;

void keyboard_handle(uint8_t scancode) {
    // key release (bit 7 set)
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A)
            shift_left = 0;
        else if (released == 0x36)
            shift_right = 0;
        return;
    }

    // shift press, tracked per key so releasing one leaves the other held
    if (scancode == 0x2A) {
        shift_left = 1;
        return;
    }
    if (scancode == 0x36) {
        shift_right = 1;
        return;
    }

    if (scancode >= 128)
        return;

    const char *map = (shift_left || shift_right) ? scancode_map_shift : scancode_map;
    char c = map[scancode];
    if (c == 0)
        return;

    input_handle_char(c);
}
