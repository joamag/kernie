/**
 * kernel/arch.h
 *
 * The hooks every architecture has to provide.
 *
 * Five functions is the whole surface between portable code and the machine.
 * Keeping it this narrow is what allows the tree to carry two targets without
 * a single conditional, since the choice is made by which files the build
 * links rather than by branching inside shared ones.
 *
 * arch_idle is called by the idle loop after the input queue is drained. An
 * architecture with working interrupts halts there until the next one, while
 * one without them uses the call to poll its input instead.
 */

#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>

/**
 * Brings up whatever the architecture needs before the idle loop runs,
 * typically the interrupt controller. May legitimately do nothing.
 */
void arch_init(void);

/**
 * Called by the idle loop once the input queue has been drained.
 *
 * An architecture with interrupts should halt here until the next one
 * arrives. One without them has to poll its input instead, which is why this
 * is a call rather than a halt written into the loop itself.
 */
void arch_idle(void);

/**
 * Restarts the machine. Does not return when it succeeds.
 */
void arch_reboot(void);

/**
 * Powers the machine off. Does not return when it succeeds.
 */
void arch_shutdown(void);

/**
 * Ticks counted since boot, or zero where no timer is running yet.
 */
uint64_t arch_ticks(void);

#endif
