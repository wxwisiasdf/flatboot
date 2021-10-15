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

__attribute__((aligned(4))) static uint32_t all_io_int = 0xFF000000;
extern unsigned char disk_buffer[];
static struct css_schid schid;

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

#include <stddef.h>
int find_file(
    struct fdscb *out_dscb,
    const char *name)
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

    char tmpbuf[20];

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
                        kprintf("**** REPORT for %s ****\n", &dscb1.ds1dsnam);
                        kprintf("DSCB CYL=%u-%u, HEAD=%u-%u\n", (unsigned)dscb1.start_cc,
                            (unsigned)dscb1.end_cc, (unsigned)dscb1.start_hh, (unsigned)dscb1.end_hh);

                        out_dscb->cyl = dscb1.start_cc;
                        out_dscb->head = dscb1.start_hh;
                        out_dscb->rec = 1;
                        break;
                    }
                } else if(dscb1.ds1dsnam[0] == '\0') {
                    r = -1;
                    break;
                }
            }
            ++rec;
        }
    }

    if(r <= 0) {
        return -1;
    }
    return 0;
}

int load_file(
    struct css_schid schid,
    const char *name,
    void *buffer)
{
    uint8_t *c_buffer = (uint8_t *)buffer;
    struct fdscb fdscb;
    size_t trk_len = 18452;
    int r, errcnt = 0;

    r = find_file(&fdscb, name);
    if(r != 0) {
        return 0;
    }

    // Read until I/O error
    while(errcnt < 4
    || ((ptrdiff_t)c_buffer - (ptrdiff_t)buffer) >= 65536) {
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

        errcnt = 0;
        c_buffer += trk_len;
        fdscb.rec++;
    }

    return (int)((ptrdiff_t)c_buffer - (ptrdiff_t)buffer);
}

int load_file_or_sequence(
    struct css_schid schid,
    const char *name,
    void *buffer)
{
    uint8_t *c_buffer = (uint8_t *)buffer;
    size_t end_name = strlen(name);
    int total_read = 0;
    char tmpbuf[24] = {0};
    int r;

    memcpy(&tmpbuf[0], name, end_name + 1);
    tmpbuf[end_name + 0] = '\0';

    /* File may actually be a sequence */
    int seq_num = 0;

    /* A sequence of datasets is identified by NAME.XXX
        * where XXX => number from 000 to 999, however because
        * nobody will ever have 128 sequence files we will only allow
        * from 0 to 9, hopefully that is enough!!! */
    tmpbuf[end_name + 0] = '.';
    while(seq_num <= 9) {
        tmpbuf[end_name + 1] = '0' + seq_num;
        tmpbuf[end_name + 2] = '\0';

        r = load_file(schid, &tmpbuf[0], c_buffer);
        if(r == 0) {
            break;
        }

        kprintf("Sequence dataset: %s (%i characters)\n", tmpbuf, r);

        c_buffer += r;
        total_read += r;

        seq_num++;
    }
    return total_read;
}

int main(
    void)
{
    struct fdscb fdscb;
    signed int r;

    /*schid.id = 1;
    schid.num = 1;*/

    memcpy(&schid, (void *)0xB8, 4);

    kprintf("FLATBOOT: 390 bootloader\n");

    read_disk(schid, 0, 0, 3, &disk_buffer, 4096);

    /* Maybe the kernel is an ELF file */
    r = find_file(&fdscb, "KERNEL.ELF");
    if(r == 0) {
        kprintf("Loadinf elf format kernel\n");

        load_file_or_sequence(schid, "KERNEL.ELF", &disk_buffer);
        kprintf("Kernel has a size of %zu @ %p\n", (size_t)r, &disk_buffer);
        load_kernel(&disk_buffer);
    }

    /* Or the kernel is a BIN file? */
    r = find_file(&fdscb, "KERNEL.BIN");
    if(r == 0) {
        void (*entry)(void) = &disk_buffer;
        kprintf("Loading raw binary kernel\n");

        load_file_or_sequence(schid, "KERNEL.BIN", &disk_buffer);

        kprintf("Entry at @ %p\n", entry);
        entry();
    }

    while(1);
}