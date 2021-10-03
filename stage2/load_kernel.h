#ifndef LOAD_KERNEL_H
#define LOAD_KERNEL_H

#include <stdint.h>
#include <stddef.h>

struct elf32_hdr {
    uint8_t id[4];
    uint8_t bits;
    uint8_t endian;
    uint8_t hdr_version;
    uint8_t abi;
    uint64_t unused;
    uint16_t type;
    uint16_t instr_set;
    uint32_t version;
    uint32_t entry;
    uint32_t prog_tab;
    uint32_t sect_tab;
    uint32_t flags;
    uint16_t hdr_size;
    uint16_t prog_tab_entry_size;
    uint16_t n_prog_tab_entry;
    uint16_t sect_tab_entry_size;
    uint16_t n_sect_tab_entry;
    uint16_t str_shtab_idx;
};

struct elf32_shdr {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
};

struct elf32_phdr {
    uint32_t type;
    uint32_t offset;
    uint32_t v_addr;
    uint32_t p_addr;
    uint32_t file_size;
    uint32_t mem_size;
    uint32_t flags;
    uint32_t align;
};

struct elf32_symbol {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t section_idx;
};

struct elf32_rel {
    uint32_t offset;
    uint32_t info;
};

struct elf32_rela {
    uint32_t offset;
    uint32_t info;
    int32_t addend;
};

enum elf_symbol_bindings {
    STB_LOCAL = 0,
    STB_GLOBAL = 1,
    STB_WEAK = 2
};

enum elf_symbol_types {
    STT_NOTYPE = 0,
    STT_OBJECT = 1,
    STT_FUNC = 2
};

enum elf_section_types {
    SHT_PROGRAM_BIT = 0x01,
    SHT_SYMBOL_TABLE = 0x02,
    SHT_STRING_TABLE = 0x03
};

enum elf_section_flags {
    SHF_WRITABLE = 0x01,
    SHF_ALLOCATE = 0x02,
    SHF_STRINGS = 0x20
};

enum elf_reloc_types {
    R_390_NONE = 0,
    R_390_8 = 1,
    R_390_12 = 2,
    R_390_16 = 3,
    R_390_32 = 4,
    R_390_PC32 = 5,
    R_390_GOT12 = 6,
    R_390_GOT32 = 7,
    R_390_PLT32 = 8,
    R_390_COPY = 9,
    R_390_GLOB_DAT = 10,
    R_390_JMP_SLOT = 11,
    R_390_RELATIVE = 12,
    R_390_GOTOFF = 13,
    R_390_GOTPC = 14,
    R_390_GOT16 = 15,
    R_390_PC16 = 16,
    R_390_PC16DBL = 17,
    R_390_PLT16DBL = 18
};

#endif