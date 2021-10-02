#include <load_kernel.h>
#include <string.h>
#include <stivale2.h>

/* The minimum allowed address (below this point the kernel is rejected) */
const size_t min_kernal_addr = 18452;

static struct elf32_shdr *elf32_get_shdr(struct elf32_hdr *hdr, int idx) {
    struct elf32_shdr *shtable = (struct elf32_shdr *)((uintptr_t)hdr + hdr->sect_tab);
    return (struct elf32_shdr *)(
        (uintptr_t)shtable +
        ((uintptr_t)idx * hdr->sect_tab_entry_size)
    );
}

static struct elf32_phdr *elf32_get_phdr(struct elf32_hdr *hdr, int idx) {
    struct elf32_phdr *phtable = (struct elf32_phdr *)((uintptr_t)hdr + hdr->prog_tab);
    return (struct elf32_phdr *)(
        (uintptr_t)phtable +
        ((uintptr_t)idx * hdr->prog_tab_entry_size)
    );
}

static struct elf32_shdr *elf32_get_string_section(struct elf32_hdr *hdr) {
    if(hdr->str_shtab_idx >= hdr->n_sect_tab_entry) {
        size_t i;

        /* Manually search */
        for(i = 0; i < hdr->n_sect_tab_entry; i++) {
            struct elf32_shdr *shdr = elf32_get_shdr(hdr, i);
            if(shdr->type == SHT_STRING_TABLE) {
                return shdr;
            }

            if(shdr->flags & SHF_STRINGS != 0) {
                return shdr;
            }

            if(shdr->type == 1 && shdr->flags == 48) {
                kprintf("Using .debug_str but without unique identifier, weird strings WILL appear!");
                return shdr;
            }
        }
        return NULL;
    }
    return elf32_get_shdr(hdr, hdr->str_shtab_idx);
}

static char *elf32_get_string(struct elf32_hdr *hdr, int off) {
    struct elf32_shdr *shdr = elf32_get_string_section(hdr);
    if(shdr == NULL) {
        return NULL;
    }
    return (char *)((uintptr_t)hdr + (uintptr_t)shdr->offset + (uintptr_t)off);
}

/* Because EBCDIC is superior :D */
#define ASCII_STIVALE2_HDR "\x2e\x73\x74\x69\x76\x61\x6c\x65\x32\x68\x64\x72"

typedef void (*stivale2_entry_t)(struct stivale2_struct *);

int load_kernel(void *data) {
    struct elf32_hdr *hdr = (struct elf32_hdr *)data;
    uintptr_t stack_top = 18452, entry_point = 0;
    size_t i, j;
    
    kprintf("Entry: %p\n", (uintptr_t)hdr->entry);
    kprintf("StringShdrOffset: %u\n", (unsigned)hdr->str_shtab_idx);
    kprintf("SectTable: Size=%u,Num=%u,Offset=%u\n", (unsigned)hdr->sect_tab_entry_size, (unsigned)hdr->n_sect_tab_entry, (unsigned)hdr->sect_tab);

    kprintf("%e\n", (uintptr_t)hdr + (uintptr_t)elf32_get_string_section(hdr)->offset);

    for(i = 0; i < hdr->n_prog_tab_entry; i++) {
        struct elf32_phdr *phdr = elf32_get_phdr(hdr, i);
        kprintf("(Physical) Address=%u,Size=%u (Virtual) Address=%u,Size=%u\n", (unsigned)phdr->p_addr,
            (unsigned)phdr->file_size, (unsigned)phdr->v_addr, (unsigned)phdr->mem_size);
    }

    for(i = 0; i < hdr->n_sect_tab_entry; i++) {
        struct elf32_shdr *shdr = elf32_get_shdr(hdr, i);

        /* Ignore invalid sections */
        if(shdr->size == 0) {
            continue;
        }

        /* If it's a symbol table we will look up stuff starting with stack_top or stack */
        if(shdr->type == SHT_SYMBOL_TABLE) {
            for(j = 0; j < shdr->size / sizeof(struct elf32_symbol); j++) {
                struct elf32_symbol *sym = &((struct elf32_symbol *)((uintptr_t)data + shdr->offset))[j];
                kprintf("Name of symbol: %e\n", elf32_get_string(hdr, sym->name));
            }
        }

        /* Ignore stuff like the debugging sections and so on */
        if((shdr->type == SHT_PROGRAM_BIT && (shdr->flags == 0 || shdr->flags == 48))
        || shdr->type == SHT_SYMBOL_TABLE
        || shdr->type == SHT_STRING_TABLE) {
            continue;
        }

        kprintf("Name=%e, Address=%p(FILE %u), Size=%u, Type=%u, Flags=%u\n",
            elf32_get_string(hdr, shdr->name), (uintptr_t)shdr->addr, (unsigned)shdr->offset,
            (unsigned)shdr->size, (unsigned)shdr->type, (unsigned)shdr->flags);
        
        /*if((uintptr_t)shdr->addr <= min_kernal_addr) {
            kprintf("Kernel violates bootloader space\n");
            while(1);
        }*/

        /* Place the data from the disk buffer to the real address */
        memcpy((void *)shdr->addr, (const void *)((uintptr_t)data + shdr->offset), shdr->size);

        /* Stivale2 header */
        if(!memcmp(elf32_get_string(hdr, shdr->name), ASCII_STIVALE2_HDR, 12)) {
            struct stivale2_header *st2hdr = (struct stivale2_header *)shdr->addr;
            stack_top = st2hdr->stack;
            entry_point = st2hdr->entry_point;

            kprintf("Stivale 2 header\n");
        }
    }

    if(entry_point == 0) {
        entry_point = hdr->entry;
    }

    /* TODO: We should load modules and such */

    kprintf("Loading kernel\n");
    
    stivale2_entry_t entry = (stivale2_entry_t)entry_point;

    /* Set the stack */
    __asm__ __volatile__(
        "la 15, 0(%0)\n"
        "br %1\n"
        :
        : "r"(stack_top), "r"(entry)
        :);
    
    kprintf("Returned from kernel\n");
    while(1);
}
