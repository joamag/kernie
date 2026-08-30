#include "kernel/arch.h"
#include "kernel/input.h"
#include "drivers/serial.h"

#include <stdint.h>

void arch_init(void) {
    /* no GIC yet, input is polled from the idle loop instead */
}

void arch_idle(void) {
    int c = serial_getchar();
    if (c >= 0)
        input_handle_char((char)c);
}

/* neither PSCI nor semihosting answers on the virt machine when QEMU is
   handed an ELF, so say so rather than hanging on an unhandled exception */
void arch_reboot(void) {
    serial_print("reboot is not supported on arm64 yet\n");
}

void arch_shutdown(void) {
    serial_print("shutdown is not supported on arm64 yet\n");
}

uint64_t arch_ticks(void) {
    /* no timer yet, the generic timer is still to be wired up */
    return 0;
}
