#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include "io.h"

#define IDT_ENTRIES 256

/* IDT gate descriptor (64-bit mode) */
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) IdtEntry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IdtPtr;

void idt_set_gate(int n, uint64_t handler);
void idt_load(void);
void pic_remap(void);
void pic_send_eoi(uint8_t irq);

#endif
