ENTRY(start);
SECTIONS {
    . = 0x1000;
    .text : {
        KEEP(*(.text.init));
        *(.text.*);
    }
    .rodata : {
        *(.rodata*);
    }
    .data : {
        *(.data*);
    }

    .bss : {
        bss_start = .;
        *(COMMON);
        *(.bss*);

        stack_bottom = .;
        . += 8K;
        stack_top = .;
        bss_end = .;
    }

    . = ALIGN(4K);
    disk_buffer = .;
}
