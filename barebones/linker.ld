ENTRY(_start);
SECTIONS {
    . = 0x8000;
    .text : {
        *(.text.init);
        *(.text.*);
        *(.stivale2hdr*);
    }
    
    .rodata : {
        *(.rodata*);
    }

    .data : {
        *(.data*);
    }

    .bss : ALIGN(4K) {
        *(COMMON);
        *(.bss*);
    }
}
