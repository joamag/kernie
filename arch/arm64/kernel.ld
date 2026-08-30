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
