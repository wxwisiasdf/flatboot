ENTRY(start);
SECTIONS {
    . = 0x4000;
    .text : ALIGN(4K) {
        KEEP(*(.text.init));
        *(.text.*);
    }
    .rodata : ALIGN(4K) {
        *(.rodata*);
    }
    .data : ALIGN(4K) {
        *(.data*);
    }

    .bss : ALIGN(4K) {
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
