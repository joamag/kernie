#include "kernel/console.h"
#include "kernel/arch.h"
#include "kernel/shell.h"
#include "kernel/input.h"
#include "lib/mem.h"

/* placed by the linker script, the loaded image carries no .bss so it holds
   whatever was in memory until we clear it */
extern uint8_t __bss_start[], __bss_end[];

__attribute__((section(".text.kernel_main"))) void kernel_main(void) {
    memset(__bss_start, 0, (uintptr_t)__bss_end - (uintptr_t)__bss_start);

    console_init();
    console_clear();

    /* bring the shell up first, so the banner cannot race an early IRQ */
    shell_init();
    arch_init();

    /* idle loop */
    for (;;) {
        input_poll();
        arch_idle();
    }
}
