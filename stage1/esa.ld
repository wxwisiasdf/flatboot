ENTRY(start);

SECTIONS {
    . = 0;
    .text : {
        *(.text.init);
        . = 1024;
        *(.text*);
    }
    .rodata : {
        *(.rodata*);
    }
    .data : {
        *(.data*);
    }
    .bss : {
        *(COMMON);
        *(.bss*);
    }

    . = 0x4000;
    disk_buffer = .;
}
