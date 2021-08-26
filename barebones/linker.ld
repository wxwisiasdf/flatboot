ENTRY(start);
SECTIONS {
    . = 0x8000;
    .text : {
        *(.text.init);

        stivale2hdr_start = .;
        *(.stivale2hdr*);
        *(.text.*);
    }
    .rodata : {
        *(.rodata*);
    }
    .data : {
        *(.data*);
    }

    .bss : ALIGN(4K) {
        bss_start = .;
        *(COMMON);
        *(.bss*);

        stack_bottom = .;
        . += 4K;
        stack_top = .;
        bss_end = .;
    }
}
