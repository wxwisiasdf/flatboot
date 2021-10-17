/* 2703 TERMINAL DRIVER FOR TELNET STUFF */

#include <stdint.h>
#include <string.h>
#include <css.h>
#include <stddef.h>
#include <dasd.h>
#include <psw.h>

__attribute__((aligned(4))) static uint32_t all_io_int = 0xFF000000;

/* fuck it im hardcoding it! */
static struct css_ccw1 tn_cchain[8] = {
    {
        .cmd = 0x27, /* ENABLE */
        .addr = (uint32_t)0,
        /* this is chained and we can skip it */
        .flags = 0x40 | 0x20,
        .length = 0,
    },
    {
        .cmd = 0x01, /* WRITE */
        .addr = (uint32_t)0,
        .flags = 0,
        .length = 0,
    }
};
static struct css_orb rb_orb = {
    .reserved1 = 0,
    .lpm = 0x0080FF00,
    .addr = (uint32_t)&tn_cchain[0],
    .reserved2 = {0}
};
static struct css_irb rb_irb = {0};

void telnet_term_write(const char *data, size_t len) {
    static struct s390_psw rb_wtnoer = {0x060E0000, AMBIT};
#if (MACHINE <= M_S390)
    static struct s390_psw rb_newio = {0x000C0000, AMBIT + (uint32_t) && rb_count};
#else
    static struct s390x_psw rb_newio = {0x00040000 | AM64, AMBIT, 0, (uint32_t) && rb_count};
#endif
    /* HARDCODED TERMINAL ADDRESS */
    struct css_schid schid = { 1, 0 };

    tn_cchain[1].addr = (uint32_t)data;
    tn_cchain[1].length = len;

#if (MACHINE <= M_S390)
    *((volatile uint64_t *)FLCINPSW) = *((uint64_t *)&rb_newio);
#else
    memcpy((void *)FLCEINPSW, &rb_newio, 16);
#endif

    __asm__ __volatile__("stosm 0x78, 0x00");
    *((volatile uint32_t *)FLCCAW) = (uint32_t)&tn_cchain[0];
    css_start_channel(schid, &rb_orb);
    __asm__ goto("lpsw %0\n" : : "m"(rb_wtnoer) : : rb_count);
rb_count:
    css_test_channel(schid, &rb_irb);
    if(rb_irb.scsw.cpa_addr != (uint32_t)&tn_cchain[4]) {
        kprintf(data);
        return;
    }
    return;
}