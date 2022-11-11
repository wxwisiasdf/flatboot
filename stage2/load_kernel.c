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

static char *elf32_get_string(struct elf32_hdr *hdr, struct elf32_shdr *shdr, int off) {
    if(shdr == NULL) {
        return NULL;
    }
    return (char *)((uintptr_t)hdr + (uintptr_t)shdr->offset + (uintptr_t)off);
}

/* Because EBCDIC is superior :D */
#define ASCII_STIVALE2_HDR "\x2e\x73\x74\x69\x76\x61\x6c\x65\x32\x68\x64\x72"

typedef void (*stivale2_entry_t)(struct stivale2_struct *);

void hdebug_term_write(const char *data, size_t len) {
    kprintf(data);
    return;
}

/* Im not going to make a proper allocation manager no, fuck that */
struct stivale2_struct_any {
    union {
        struct stivale2_struct_tag_pmrs pmrs;
        struct stivale2_struct_tag_cmdline cmdline;
        struct stivale2_struct_tag_memmap memmap;
        struct stivale2_struct_tag_framebuffer fb;
        struct stivale2_struct_tag_edid edid;
        struct stivale2_struct_tag_textmode textmode;
        struct stivale2_struct_tag_terminal term;
        struct stivale2_struct_tag_modules modules;
        struct stivale2_struct_tag_rsdp rdsp;
        struct stivale2_struct_tag_epoch epoch;
        struct stivale2_struct_tag_firmware firmware;
        struct stivale2_struct_tag_efi_system_table efi_systab;
        struct stivale2_struct_tag_kernel_file kfile_v1;
        struct stivale2_struct_tag_kernel_file_v2 kfile_v2;
        struct stivale2_struct_tag_kernel_slide slide;
        struct stivale2_struct_tag_smbios smbios;
        struct stivale2_struct_tag_smp smp;
        struct stivale2_struct_tag_pxe_server_info pxe_server;
        struct stivale2_struct_tag_mmio32_uart uart;
        struct stivale2_struct_tag_dtb dtb;
        struct stivale2_struct_tag_hhdm hhdm;
        struct stivale2_header_tag_slide_hhdm slide_hhdm;
        struct stivale2_struct_tag_kernel_base_address kernel_base_address;
    };
};

static struct stivale2_struct st2_boot_cfg = {
    .bootloader_brand = "ENTERPRISE SYSTEM ARCHITECTURE 360, 370 AND 390 BOOTLOADER",
    .bootloader_version = "FRAMEBOOT VERSION 1.0, PRE-ALPHA",
    .tags = 0,
};

/* --- TELNET TERMINAL DEVICE --- */
static struct stivale2_struct_tag_terminal bc_telnet_term = {
    .tag.identifier = STIVALE2_STRUCT_TAG_TERMINAL_ID,
    .tag.next = 0,
    .cols = 0,
    .rows = 0,
    .max_length = 80,
    .term_write = 0
};

/* --- 3270 TERMINAL DEVICE --- */
static struct stivale2_struct_tag_terminal bc_3270_term = {
    .tag.identifier = STIVALE2_STRUCT_TAG_TERMINAL_ID,
    .tag.next = 0,
    .cols = 80,
    .rows = 24,
    .max_length = 80,
    .term_write = 0
};

/* --- HERCULES DEBUG CONSOLE --- */
static struct stivale2_struct_tag_terminal bc_herc_term = {
    .tag.identifier = STIVALE2_STRUCT_TAG_TERMINAL_ID,
    .tag.next = 0,
    .cols = 80,
    .rows = 40,
    .max_length = 80,
    .term_write = 0
};

static struct stivale2_struct_tag_firmware bc_firmware = {
    .tag.identifier = STIVALE2_STRUCT_TAG_FIRMWARE_ID,
    .tag.next = 0,
    .flags = 0  
};

static struct stivale2_struct_tag_kernel_base_address bc_kernel_base_address = {
    .tag.identifier = STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID,
    .tag.next = 0,
    .physical_base_address = 0,
    .virtual_base_address = 0
};

static struct stivale2_struct_tag_kernel_file bc_kernfile = {
    .tag.identifier = STIVALE2_STRUCT_TAG_KERNEL_FILE_ID,
    .tag.next = 0,
};

void telnet_term_write(const char *data, size_t len);

int load_kernel(
    void *data)
{
    struct elf32_hdr *hdr = (struct elf32_hdr *)data;
    uintptr_t stack_top = 18452, entry_point = 0;
    size_t i, j;

    bc_kernfile.tag.next = 0;

    bc_kernel_base_address.tag.next = &bc_kernfile;
    
    bc_firmware.tag.next = &bc_kernel_base_address;

    bc_telnet_term.tag.next = &bc_firmware;
    bc_telnet_term.term_write = (uint64_t)&telnet_term_write;

    bc_3270_term.tag.next = &bc_telnet_term;
    bc_3270_term.term_write = (uint64_t)&telnet_term_write;

    bc_herc_term.tag.next = &bc_3270_term;
    bc_herc_term.term_write = (uint64_t)&hdebug_term_write;

    st2_boot_cfg.tags = &bc_herc_term;

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
            struct elf32_symbol *symtab = (struct elf32_symbol *)((uintptr_t)data + shdr->offset);
            for(j = 0; j < shdr->size / sizeof(struct elf32_symbol); j++) {
                struct elf32_symbol *sym = &symtab[j];

                kprintf("Name of symbol: %e\n", elf32_get_string(hdr, elf32_get_string_section(hdr), sym->name));
                kprintf("Info=%zu, Other=%zu, Sh=%zu, Size=%zu, Value=%zu\n", (size_t)sym->info, (size_t)sym->other, (size_t)sym->section_idx, (size_t)sym->size, (size_t)sym->value);
            }
        }

        kprintf("Name=%e, Address=%p(FILE %u), Size=%u, Type=%u, Flags=%u\n",
            elf32_get_string(hdr, elf32_get_string_section(hdr), shdr->name), (uintptr_t)shdr->addr, (unsigned)shdr->offset,
            (unsigned)shdr->size, (unsigned)shdr->type, (unsigned)shdr->flags);
        
        if((uintptr_t)shdr->addr <= min_kernal_addr) {
            kprintf("Kernel violates bootloader space\n");
            while(1);
        }

        /* Place the data from the disk buffer to the real address */
        memcpy((void *)shdr->addr, (const void *)((uintptr_t)data + shdr->offset), shdr->size);

        /* Stivale2 header */
        if(!memcmp(elf32_get_string(hdr, elf32_get_string_section(hdr), shdr->name), ASCII_STIVALE2_HDR, 12)) {
            struct stivale2_header *st2hdr = (struct stivale2_header *)shdr->addr;
            stack_top = st2hdr->stack;
            entry_point = st2hdr->entry_point;
            
            kprintf("Stivale 2 header\n");
            /* TODO: I think we are supposed to do something with this section :S */
        }
    }

    if(entry_point == 0) {
        entry_point = hdr->entry;
    }

    /* TODO: Use the stack of the kernel instead of ours */
    stivale2_entry_t entry = (stivale2_entry_t)entry_point;

    /* TODO: We should load modules and such for the kernel */
    kprintf("Loading kernel with entry @ %p\n", entry);
    entry(&st2_boot_cfg);
    
    kprintf("Returned from kernel\n");
    while(1);
}
