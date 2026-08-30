#include "kernel/input.h"
#include "kernel/shell.h"

#define INPUT_MAX 256

/* filled from interrupt context and drained by input_poll() in the idle loop,
   so the shell never runs behind a closed interrupt gate */
static volatile char input_buf[INPUT_MAX];
static volatile int input_head = 0;
static volatile int input_tail = 0;

void input_handle_char(char c) {
    int next = (input_head + 1) % INPUT_MAX;

    /* drop the byte rather than overwrite one the shell has not read yet */
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
