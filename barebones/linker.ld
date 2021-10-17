ENTRY(_start);
SECTIONS {
    . = 0x20000;
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
