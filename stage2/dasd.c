#include <stdint.h>
#include <string.h>
#include <css.h>
#include <stddef.h>
#include <dasd.h>
#include <psw.h>

/* It works (kinda), so shut up */
__attribute__((aligned(8))) static struct {
    uint16_t bb;
    uint16_t cc;
    uint16_t hh;
    uint8_t record;
}read_block = {0};
static struct css_ccw1 rb_cchain[8] = {
    {
        .cmd = 0x07, /* SEEK */
        .addr = (uint32_t)&read_block.bb,
        .flags = 0x40,
        .length = 6,
    },
    {
        .cmd = 0x31, /* SEARCH */
        .addr = (uint32_t)&read_block.cc,
        .flags = 0x40,
        .length = 5,
    },
    {
        .cmd = 0x08, /* TIC */
        .addr = (uint32_t)&rb_cchain[1],
        .flags = 0x00,
        .length = 0x00,
    },
    {
        .cmd = 0x0e, /* READ KEY AND DATA */
        .addr = (uint32_t)0,
        .flags = 0x20,
        .length = (uint16_t)32767,
    }
};
static struct css_orb rb_orb = {
    .reserved1 = 0,
    .lpm = 0x0080FF00,
    .addr = (uint32_t)&rb_cchain[0],
    .reserved2 = {0}
};
static struct css_irb rb_irb = {0};

/*
 * Read by CHR from disk given by SCHID
 */
int read_disk(struct css_schid schid, uint16_t cyl, uint16_t head, uint8_t rec,
                             void *buf, size_t n) {
    static struct s390_psw rb_wtnoer = {0x060E0000, AMBIT};
#if (MACHINE <= M_S390)
    static struct s390_psw rb_newio = {0x000C0000, AMBIT + (uint32_t) && rb_count};
#else
    static struct s390x_psw rb_newio = {0x00040000 | AM64, AMBIT, 0, (uint32_t) && rb_count};
#endif
    int r;

    read_block.bb = 0;
    read_block.cc = cyl;
    read_block.hh = head;
    read_block.record = rec;

    rb_cchain[3].addr = (uint32_t)buf;
    rb_cchain[3].length = (uint16_t)n;

#if (MACHINE <= M_S390)
    *((volatile uint64_t *)FLCINPSW) = *((uint64_t *)&rb_newio);
#else
    memcpy((void *)FLCEINPSW, &rb_newio, 16);
#endif

    __asm__ __volatile__("stosm 0x78, 0x00");
    *((volatile uint32_t *)FLCCAW) = (uint32_t)&rb_cchain[0];
    r = css_test_channel(schid, &rb_irb);
    if(r != 0) {
        /*diag8_write("Test channel error", 19);
        return -1;*/
    }

    r = css_start_channel(schid, &rb_orb);
    if(r != 0) {
        diag8_write("Start channel error", 20);
        return -1;
    }
    __asm__ goto("lpsw %0\n" : : "m"(rb_wtnoer) : : rb_count);
rb_count:
    r = css_test_channel(schid, &rb_irb);
    if(r != 0) {
        diag8_write("Test channel error", 19);
        return -1;
    }

    if(rb_irb.scsw.cpa_addr != (uint32_t)&rb_cchain[4]) {
        diag8_write("Channel program did not reach FINCHAIN", 39);
        return -1;
    }
    return (int)n - (int)rb_irb.scsw.count;
}