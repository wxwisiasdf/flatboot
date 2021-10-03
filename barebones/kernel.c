/*
    Licensed under 0BSD. See https://github.com/limine-bootloader/limine-barebones/blob/master/LICENSE.md

    Original:
    https://github.com/limine-bootloader/limine-barebones/blob/master/stivale2-barebones/kernel/kernel.c

    Modifications done for EBCDIC compatibility
*/

#include <stdint.h>
#include <stddef.h>
#include "../stage2/stivale2.h"

struct stivale2_struct_tag_terminal *term = NULL;

void _start(struct stivale2_tag *tag) {
    while(tag != NULL) {
        if(tag->identifier == STIVALE2_STRUCT_TAG_TERMINAL_ID) {
            term = tag;
        }

        tag = (struct stivale2_tag *)tag->next;
    }

    void (*term_write)(const char *string, size_t length) = term->term_write;
    term_write("Hello world!\n", 12);

	while(1);
}
