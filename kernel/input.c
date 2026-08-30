/**
 * kernel/input.c
 *
 * Ring buffer between interrupt context and the shell.
 *
 * Both the PS/2 keyboard and the serial port funnel through input_handle_char,
 * which only enqueues, while input_poll drains the queue from the idle loop
 * with interrupts enabled.
 *
 * The indirection is what keeps command execution out of the interrupt gate. A
 * command such as help writes a few hundred bytes through a polled UART, and
 * doing that inside the handler would hold interrupts off long enough for a
 * pasted burst to overrun the sixteen byte receive FIFO and for timer ticks to
 * be missed.
 *
 * A full buffer drops the byte rather than overwriting one the shell has not
 * read yet, since losing the newest keystroke is easier to notice and easier
 * to explain than corrupting a command already half typed.
 */

#include "kernel/input.h"
#include "kernel/shell.h"

#define INPUT_MAX 256

// filled from interrupt context and drained by input_poll() in the idle loop,
// so the shell never runs behind a closed interrupt gate
static volatile char input_buf[INPUT_MAX];
static volatile int input_head = 0;
static volatile int input_tail = 0;

void input_handle_char(char c) {
    int next = (input_head + 1) % INPUT_MAX;

    // drop the byte rather than overwrite one the shell has not read yet
    if (next == input_tail)
        return;

    input_buf[input_head] = c;
    input_head = next;
}

void input_poll(void) {
    while (input_tail != input_head) {
        char c = input_buf[input_tail];
        input_tail = (input_tail + 1) % INPUT_MAX;
        shell_handle_char(c);
    }
}
