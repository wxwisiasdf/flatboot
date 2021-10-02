#ifndef DASD_H
#define DASD_H

#include <stdint.h>
#include <stddef.h>

int read_disk(struct css_schid schid, uint16_t cyl, uint16_t head, uint8_t rec, void *buf, size_t n);

#endif