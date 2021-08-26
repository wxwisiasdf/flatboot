#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "../stage2/stivale2.h"


#define STIVALE_ANCHOR "\x53\x54\x49\x56\x41\x4C\x45\x32\x20\x41\x4E\x43\x48\x4F\x52"

extern unsigned bss_start;
extern unsigned bss_end;
extern unsigned stivale2hdr_start;

void start(void) {
    while(1);
    return;
}

__attribute__((section(".stivale2hdr"))) struct stivale2_header st2head = {
    .entry_point = 0,
    .stack = 0,
    .flags = 0,
    .tags = 0,
};