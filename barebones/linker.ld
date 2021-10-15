ENTRY(_start);
SECTIONS {
    . = 0xf000;
    .text : {
        *(.text);
        *(.text.*);
        *(.stivale2hdr*);
    }
    
    .rodata : {
        *(.rodata);
        *(.rodata.*);
    }

    .data : {
        *(.data);
        *(.data.*);
    }

    .bss : ALIGN(4K) {
        *(COMMON);
        *(.bss);
        *(.bss.*);
    }
}
