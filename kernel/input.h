/**
 * kernel/input.h
 *
 * The single sink for every input source, and the drain the idle loop calls.
 */

#ifndef INPUT_H
#define INPUT_H

/**
 * Queues a byte from an input source. Safe to call from interrupt context,
 * and drops the byte when the queue is full rather than overwriting one the
 * shell has not read yet.
 */
void input_handle_char(char c);

/**
 * Drains the queue into the shell. Called from the idle loop with interrupts
 * enabled, never from a handler.
 */
void input_poll(void);

#endif
