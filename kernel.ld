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
        *(.bss)
        __bss_end = .;
    }
}
