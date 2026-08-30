#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>

void arch_init(void);
void arch_idle(void);
void arch_reboot(void);
void arch_shutdown(void);
uint64_t arch_ticks(void);

#endif
