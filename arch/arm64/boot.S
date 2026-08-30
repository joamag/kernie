// boot.S - entry point for the virt machine
// QEMU loads the ELF and jumps here, so there is no real mode dance

.section .text.boot
.global _start

_start:
    // park every secondary core, only cpu 0 continues
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF
    cbnz    x0, park

    ldr     x0, =__stack_top
    mov     sp, x0

    bl      kernel_main

park:
    wfe
    b       park
