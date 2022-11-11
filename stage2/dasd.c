#include <stdint.h>
#include <string.h>
#include <css.h>
#include <stddef.h>
#include <dasd.h>
#include <psw.h>

__attribute__((aligned(4))) static uint32_t all_io_int = 0xFF000000;
struct css_schid schid;

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
    if(r == 3) {
        kprintf("Test channel error\n");
        return -1;
    }

    r = css_start_channel(schid, &rb_orb);
    if(r == 3) {
        kprintf("Start channel error\n");
        return -1;
    }
    __asm__ goto("lpsw %0\n" : : "m"(rb_wtnoer) : : rb_count);
rb_count:
    r = css_test_channel(schid, &rb_irb);
    if(r == 3) {
        kprintf("Test channel error\n");
        return -1;
    }

    if(rb_irb.scsw.cpa_addr != (uint32_t)&rb_cchain[4]) {
        return -1;
    }
    return (int)n - (int)rb_irb.scsw.count;
}

int find_file(struct fdscb *out_dscb, const char *name)
{
    /* If we make a struct like this gcc generates code which incorrectly
     * handle start_cc and start_hh */
    /* __attribute__((packed)) struct {
        char ds1dsnam[44];
        char ds1fmtid;
        uint8_t unused[60];
        uint8_t ext_type;
        uint8_t ext_seq_num;
        uint16_t start_cc;
        uint16_t start_hh;
        uint16_t end_cc;
        uint16_t end_hh;
    } dscb1; */

    /* We have to make it like this */
    struct dscb_fmt1 dscb1;
    char tmpbuf[24];

    int cyl, head, rec;
    int r = 0, errcnt = 0;
    struct fdscb fdscb;

    r = read_disk(schid, 0, 0, 3, &tmpbuf[0], 20);
    if(r >= 20) {
        memcpy(&fdscb.cyl, &tmpbuf[15], 2); /* 15-16 */
        memcpy(&fdscb.head, &tmpbuf[17], 2); /* 17-18 */
        memcpy(&fdscb.rec, &tmpbuf[19], 1); /* 19-19 */
        cyl = (int)fdscb.cyl;
        head = (int)fdscb.head;
        rec = (int)fdscb.rec;

        while(errcnt < 4) {
            r = read_disk(schid, (uint16_t)cyl, (uint16_t)head, (uint8_t)rec, &dscb1, sizeof(dscb1));
            if(r < 0) {
                ++errcnt;
                if(errcnt == 1) {
                    ++rec;
                } else if(errcnt == 2) {
                    rec = 1;
                    ++head;
                } else if(errcnt == 3) {
                    rec = 1;
                    head = 0;
                    ++cyl;
                }
                continue;
            }

            errcnt = 0;
            if(r >= (int)sizeof(dscb1)) {
                if(dscb1.ds1fmtid == '1') {
                    dscb1.ds1fmtid = ' ';
                    if(!memcmp(&dscb1.ds1dsnam, name, strlen(name))) {
                        kprintf("**** Report of find dataset %s job ****\n", name);
                        kprintf("DSCB CYL=%u-%u, HEAD=%u-%u\n", (unsigned)dscb1.start_cc,
                            (unsigned)dscb1.end_cc, (unsigned)dscb1.start_hh, (unsigned)dscb1.end_hh);

                        out_dscb->cyl = dscb1.start_cc;
                        out_dscb->head = dscb1.start_hh;
                        out_dscb->rec = 1;
                        return 0;
                    }
                } else if(dscb1.ds1dsnam[0] == '\0') {
                    return -1;
                }
            }
            ++rec;
        }
    } else {
        return -1;
    }
    return 0;
}

int load_file(struct css_schid schid, const char *name, void *buffer)
{
    char *c_buffer = (char *)buffer;
    struct fdscb fdscb;
    size_t trk_len = 18452;
    int r, errcnt = 0;

    r = find_file(&fdscb, name);
    if(r != 0) {
        return -1;
    }

    /* Read until I/O error */
    while(errcnt < 4) {
        r = read_disk(schid, fdscb.cyl, fdscb.head, fdscb.rec, c_buffer, trk_len);
        if(r < 0) {
            errcnt++;
            if(errcnt == 1) {
                fdscb.rec++;
            } else if(errcnt == 2) {
                fdscb.rec = 1;
                fdscb.head++;
            } else if(errcnt == 3) {
                fdscb.rec = 1;
                fdscb.head = 0;
                fdscb.cyl++;
            }
            continue;
        }
        c_buffer += trk_len;

        errcnt = 0;
        fdscb.rec++;
    }

    return (int)((ptrdiff_t)c_buffer - (ptrdiff_t)buffer);
}