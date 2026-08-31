/**
 * arch/x86_64/kernel.ld
 *
 * Linker script for the x86-64 flat binary.
 *
 * The output has no ELF headers, so there is nothing for a loader to read and
 * the bootloader simply jumps to the first byte. That is why kernel_main is
 * forced to the front of .text through its own section, and why the entry
 * point declaration below is documentation rather than something honoured.
 *
 * .bss is NOBITS and therefore absent from the image, which is what makes
 * clearing it at startup necessary. The boundary symbols are exported for
 * that.
 */

ENTRY(kernel_main)

SECTIONS {
    . = 0x100000;

    .text : {
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
}
