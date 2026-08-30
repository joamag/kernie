// arch/arm64/boot.S
//
// Entry point for the virt machine.
//
// QEMU loads the ELF and jumps straight here, so there is no boot sector, no
// mode transition and nothing to copy. The work is limited to giving the boot
// core a stack and calling into C, which clears .bss for itself.
//
// Every core enters here, so all but the first are parked. Without that they
// would each run the kernel, and two of them writing the same console and the
// same command buffer is not a failure that looks like a concurrency bug when
// you first see it.

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
