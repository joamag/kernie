/**
 * arch/riscv64/kernel.ld
 *
 * Linker script for the riscv64 ELF.
 *
 * The virt machine starts its RAM at 0x80000000 and, with no firmware in the
 * way, resets straight to it, so the image is linked there rather than at the
 * offset a supervisor mode payload would use.
 *
 * The stack is carved out after .bss so that it follows the image instead of
 * having to be kept clear of it by hand.
 */

ENTRY(_start)

SECTIONS {
    . = 0x80000000;

    .text : {
        *(.text.boot)
        *(.text.kernel_main)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        __bss_start = .;
        *(.bss .bss.* .sbss .sbss.* COMMON)
        __bss_end = .;
    }

    . = ALIGN(16);
    . += 0x4000;
    __stack_top = .;
}
