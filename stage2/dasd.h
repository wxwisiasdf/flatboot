#ifndef DASD_H
#define DASD_H

#include <stdint.h>
#include <stddef.h>
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>
#include <printf.h>
#include <string.h>
#include <css.h>

struct fdscb {
    uint16_t cyl;
    uint16_t head;
    uint8_t rec;
};

struct dscb_fmt1 {
    char ds1dsnam[44];
    char ds1fmtid;
    uint8_t unused[60];
    uint8_t ext_type;
    uint8_t ext_seq_num;
    uint16_t start_cc;
    uint16_t start_hh;
    uint16_t end_cc;
    uint16_t end_hh;
} __attribute__((packed));

int read_disk(struct css_schid schid, uint16_t cyl, uint16_t head, uint8_t rec, void *buf, size_t n);
int find_file(struct fdscb *out_dscb, const char *name);
int load_file(struct css_schid schid, const char *name, void *buffer);

extern unsigned char disk_buffer[];
extern struct css_schid schid;

#endif