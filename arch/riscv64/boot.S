// arch/riscv64/boot.S
//
// Entry point for the virt machine.
//
// QEMU is run with no firmware, so the reset vector lands here directly and
// the kernel owns machine mode from the first instruction. There is no SBI
// underneath, which is why power control talks to the test device rather than
// issuing an ecall.
//
// The boot hart id arrives in a0 and the device tree pointer in a1, both of
// which QEMU sets up whether or not firmware ran. Only hart zero continues,
// the rest are parked before they can reach the shared console and command
// buffer.

.section .text.boot
.global _start

_start:
    bnez    a0, park

    la      sp, __stack_top

    call    kernel_main

park:
    wfi
    j       park
