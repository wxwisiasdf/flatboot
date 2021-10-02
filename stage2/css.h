#ifndef CSS_H
#define CSS_H

#include <stddef.h>

struct css_ccw1 {
    uint8_t cmd;
    uint8_t flags;
    uint16_t length;
    uint32_t addr;
} __attribute__((packed, aligned(8)));

struct css_orb {
    uint32_t reserved1;
    uint32_t lpm;
    uint32_t addr;
    uint32_t reserved2[5];
} __attribute__((packed, aligned(8)));

struct css_scsw {
    uint32_t flags;
    uint32_t cpa_addr;
    uint8_t device_status;
    uint8_t subchannel_status;
    uint16_t count;
} __attribute__((packed));

struct css_irb {
    struct css_scsw scsw;
    uint32_t esw[5];
    uint32_t ecw[8];
    uint32_t emw[8];
} __attribute__((packed, aligned(4)));

struct css_schid {
    uint16_t id;
    uint16_t num;
} __attribute__((packed));

static inline int css_start_channel(struct css_schid schid, struct css_orb *schib) {
    register struct css_schid r1 __asm__("1") = schid;
    int cc = -1;
    __asm__ __volatile__(
        "ssch 0(%1)\n"
        "ipm %0\n"
                                             : "+d"(cc)
                                             : "a"(schib), "d"(r1), "m"(schib)
                                             : "cc", "memory");
    return cc >> 28;
}

static inline int css_store_channel(struct css_schid schid, struct css_schib *schib) {
    register struct css_schid r1 __asm__("1") = schid;
    int cc = -1;
    __asm__ __volatile__("stsch 0(%2)\n"
                                             "ipm %0"
                                             : "+d"(cc), "=m"(*schib)
                                             : "a"(schib), "d"(r1)
                                             : "cc", "memory");
    return cc >> 28;
}

static inline int css_modify_channel(struct css_schid schid, void *schib) {
    register struct css_schid r1 __asm__("1") = schid;
    int cc = -1;
    __asm__ __volatile__("msch 0(%1)\n"
                                             "ipm %0"
                                             : "+d"(cc)
                                             : "a"(schib), "d"(r1), "m"(schib)
                                             : "cc", "memory");
    return cc >> 28;
}

static inline int css_test_channel(struct css_schid schid, struct css_irb *schib) {
    register struct css_schid r1 __asm__("1") = schid;
    int cc = -1;
    __asm__ __volatile__("tsch 0(%2)\n"
                                             "ipm %0"
                                             : "+d"(cc), "=m"(*schib)
                                             : "a"(schib), "d"(r1)
                                             : "cc", "memory");
    return cc >> 28;
}

#endif