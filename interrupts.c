#include "vga.h"
#include "serial.h"
#include "io.h"
#include "idt.h"
#include "keyboard.h"
#include "input.h"

/* ISR stubs defined in isr.asm */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);
extern void isr32(void); extern void isr33(void); extern void isr34(void);
extern void isr35(void); extern void isr36(void); extern void isr37(void);
extern void isr38(void); extern void isr39(void); extern void isr40(void);
extern void isr41(void); extern void isr42(void); extern void isr43(void);
extern void isr44(void); extern void isr45(void); extern void isr46(void);
extern void isr47(void);

static const char *exception_names[] = {
    "Divide by zero", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bound range", "Invalid opcode", "Device N/A",
    "Double fault", "Coproc seg", "Invalid TSS", "Seg not present",
    "Stack-seg fault", "General protection", "Page fault", "Reserved",
    "x87 FP", "Alignment check", "Machine check", "SIMD FP",
    "Virtualization", "Control protection",
};

static volatile uint64_t tick_count = 0;

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} InterruptFrame;

/* called from isr_common in isr.asm */
void isr_handler(InterruptFrame *frame) {
    uint64_t n = frame->int_no;

    if (n < 32) {
        /* CPU exception */
        const char *name = n < 22 ? exception_names[n] : "Unknown";
        serial_print("EXCEPTION: ");
        serial_print(name);
        serial_print(" (#");
        serial_print_hex(n);
        serial_print(") err=");
        serial_print_hex(frame->err_code);
        serial_print(" rip=");
        serial_print_hex(frame->rip);
        serial_print("\n");

        vga_print("EXCEPTION: ", VGA_RED_ON_BLACK);
        vga_print(name, VGA_RED_ON_BLACK);
        vga_print("\n", VGA_RED_ON_BLACK);

        /* halt on exception */
        for (;;) __asm__ volatile ("hlt");
    } else if (n == 32) {
        /* IRQ 0: timer tick */
        tick_count++;
        pic_send_eoi(0);
    } else if (n == 33) {
        /* IRQ 1: keyboard */
        uint8_t scancode = inb(0x60);
        keyboard_handle(scancode);
        pic_send_eoi(1);
    } else if (n == 36) {
        /* IRQ 4: serial COM1 input */
        while (inb(0x3F8 + 5) & 0x01) {
            char c = inb(0x3F8);
            input_handle_char(c);
        }
        pic_send_eoi(4);
    } else if (n >= 34 && n <= 47) {
        /* other IRQs */
        pic_send_eoi(n - 32);
    }
}

void interrupts_init(void) {
    typedef void (*isr_fn)(void);
    isr_fn stubs[48] = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
        isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
        isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47,
    };

    pic_remap();

    for (int i = 0; i < 48; i++)
        idt_set_gate(i, (uint64_t)stubs[i]);

    idt_load();

    /* unmask IRQ 0 (timer), IRQ 1 (keyboard), IRQ 4 (COM1) */
    outb(0x21, 0xEC);
    outb(0xA1, 0xFF);

    /* enable serial receive interrupts on COM1 */
    outb(0x3F8 + 1, 0x01);

    /* enable interrupts */
    __asm__ volatile ("sti");
}
