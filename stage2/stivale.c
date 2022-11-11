/*
 * stivale1/2 protocol implementation
 * TODO: actually implement the protocol
 * TODO: load kernel from disk
 * TODO: load elf files
 * TODO: set up paging - or not :)
 * TODO: support multiboot?
 */

#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>
#include <printf.h>
#include <string.h>
#include <css.h>
#include <dasd.h>
#include <load_kernel.h>

#define STIVALE_ANCHOR "\x53\x54\x49\x56\x41\x4C\x45\x32\x20\x41\x4E\x43\x48\x4F\x52"

int main(void)
{
    struct fdscb fdscb;
    int r;

    memcpy(&schid, (void *)0xB8, 4);

    kprintf("FLATBOOT: 390 bootloader\n");
    kprintf("Boot device is %i:%i\n", (int)schid.id, (int)schid.num);

    read_disk(schid, 0, 0, 3, &disk_buffer, 4096);

    /* Or the kernel is a BIN file? */
    r = find_file(&fdscb, "KERNEL.BIN");
    if(r == 0) {
        void (*entry)(void) = &disk_buffer;
        kprintf("Loading raw binary kernel\n");

        load_file(schid, "KERNEL.BIN", &disk_buffer);

        kprintf("Entry at @ %p\n", entry);
        entry();
    }

    /* Maybe the kernel is an ELF file */
    r = find_file(&fdscb, "KERNEL.ELF");
    if(r == 0) {
        kprintf("Loading elf format kernel\n");

        load_file(schid, "KERNEL.ELF", &disk_buffer);
        kprintf("Kernel has a size of %zu @ %p\n", (size_t)r, &disk_buffer);
        load_kernel(&disk_buffer);
    }

    while(1);
}