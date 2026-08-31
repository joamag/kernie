/**
 * arch/arm64/kernel.ld
 *
 * Linker script for the arm64 ELF.
 *
 * The load address sits at the usual kernel offset into the RAM the virt
 * machine starts at 0x40000000. QEMU honours the program headers, so unlike
 * the x86-64 script the entry point here is real.
 *
 * The stack is carved out after .bss rather than placed at a fixed address, so
 * that it follows the image instead of having to be kept clear of it by
 * hand.
 */

ENTRY(_start)

SECTIONS {
    /* virt machine RAM starts at 0x40000000, this is the usual kernel offset */
    . = 0x40080000;

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
        *(.bss .bss.* COMMON)
        __bss_end = .;
    }

    . = ALIGN(16);
    . += 0x4000;
    __stack_top = .;
}
