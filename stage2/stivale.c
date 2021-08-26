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

#define AMBIT 0x80000000
#define FLCCAW 0x48
#define FLCSNPSW 0x60
#define FLCPNPSW 0x68
#define FLCINPSW 0x78
#define FLCMNPSW 0x70
#define FLCIOA 0xB8

#define STIVALE_ANCHOR "\x53\x54\x49\x56\x41\x4C\x45\x32\x20\x41\x4E\x43\x48\x4F\x52"

struct css_ccw1 {
  uint8_t cmd;
  uint8_t flags;
  uint16_t length;
  uint32_t addr;
} __attribute__((packed, aligned(8)));

struct s390_psw {
  uint32_t lo;
  uint32_t hi;
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

void *memcpy(void *dest, const void *src, size_t n) {
  const char *c_src = (const char *)src;
  char *c_dest = (char *)dest;
  while (n) {
    *(c_dest++) = *(c_src++);
    --n;
  }
  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  int diff = 0;
  const char *_s1 = (const char *)s1;
  const char *_s2 = (const char *)s2;

  while (n) {
    diff += *(_s1++) - *(_s2++);
    --n;
  }
  return diff;
}

size_t strlen(const char *s) {
  size_t i = 0;
  while (*s != '\0') {
    ++i;
    ++s;
  }
  return i;
}

int strcmp(const char *s1, const char *s2) {
  size_t n = strlen(s1);
  int diff = 0;

  while (n && *s1 != '\0' && *s2 != '\0') {
    diff += *(s1++) - *(s2++);
    --n;
  }

  if (*s1 != *s2) {
    return -1;
  }
  return diff;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  int diff = 0;

  while (n && *s1 != '\0' && *s2 != '\0') {
    diff += *(s1++) - *(s2++);
    --n;
  }

  if (*s1 != *s2 && n > 0) {
    return -1;
  }
  return diff;
}

static char numbuf[32], tmpbuf[80];
char *out_ptr = (char *)&tmpbuf[6];
static int diag8_write(const void *buf, size_t size) {
  memcpy(&tmpbuf[0], "MSG * ", 6);
  //memcpy(&tmpbuf[6], buf, size);
  tmpbuf[size + 6] = '\0';

  asm volatile("diag %0, %1, 8"
               :
               : "r"(&tmpbuf[0]), "r"(size + 6)
               : "cc", "memory");
  return 0;
}

static void kflush(void) {
  size_t i = 0;
  *out_ptr = '\0';
  out_ptr = (char *)&tmpbuf[6];
  diag8_write(out_ptr[i], strlen(out_ptr));
  return;
}

static void print_number(unsigned long val, int base) {
  char *buf_ptr = (char *)&numbuf;
  if (!val) {
    *(buf_ptr++) = '0';
  } else {
    while (val) {
      char rem = (char)(val % (unsigned long)base);
      *(buf_ptr++) = (rem >= 10) ? rem - 10 + 'A' : rem + '0';
      val /= (unsigned long)base;
    }
  }
  while (buf_ptr != (char *)&numbuf) {
    *(out_ptr++) = *(--buf_ptr);
  }
  return;
}

static void print_inumber(signed long val, int base) {
  char *buf_ptr = (char *)&numbuf;
  if (!val) {
    *(buf_ptr++) = '0';
  } else {
    while (val) {
      char rem = (char)(val % (long)base);
      *(buf_ptr++) = (rem >= 10) ? rem - 10 + 'A' : rem + '0';
      val /= base;
    }
  }

  if (val < 0) {
    *(out_ptr++) = '-';
  }
  while (buf_ptr != (char *)&numbuf) {
    *(out_ptr++) = *(--buf_ptr);
  }
  return;
}

#include <stdarg.h>
static int kprintf(const char *fmt, ...);
static int kvprintf(const char *fmt, va_list args);

static int kprintf(const char *fmt, ...) {
  va_list args;
  int r;

  va_start(args, fmt);
  r = kvprintf(fmt, args);
  va_end(args);
  return r;
}

static int kvprintf(const char *fmt, va_list args) {
  size_t i;

  while (*fmt != '\0') {
    if (*fmt == '\n') {
      *(out_ptr++) = *(fmt++);
      kflush();
      continue;
    }

    if (*fmt == '%') {
      ++fmt;
      if (!strncmp(fmt, "s", 1)) {
        const char *str = va_arg(args, const char *);
        if (str == NULL) {
          kprintf("(nil)");
        } else {
          for (i = 0; i < strlen(str); i++) {
            *(out_ptr++) = str[i];
          }
        }
      } else if (!strncmp(fmt, "zu", 2)) {
        size_t val = va_arg(args, size_t);
        print_number((unsigned long)val, 10);
        ++fmt;
      } else if (!strncmp(fmt, "u", 1)) {
        unsigned int val = va_arg(args, unsigned int);
        print_number((unsigned long)val, 10);
      } else if (!strncmp(fmt, "p", 1)) {
        unsigned val = va_arg(args, unsigned);
        print_number((unsigned long)val, 16);
      } else if (!strncmp(fmt, "x", 1)) {
        unsigned val = va_arg(args, unsigned);
        print_number((unsigned long)val, 16);
      } else if (!strncmp(fmt, "i", 1)) {
        signed int val = va_arg(args, signed int);
        print_inumber((signed long)val, 10);
      }
      ++fmt;
    } else {
      *(out_ptr++) = *(fmt++);
    }
  }
  return 0;
}

struct css_schid {
  uint16_t id;
  uint16_t num;
} __attribute__((packed));
int css_start_channel(struct css_schid schid, struct css_orb *schib) {
  register struct css_schid r1 __asm__("1") = schid;
  int cc = -1;
  __asm__ __volatile__("ssch 0(%1)\n"
                       "ipm %0\n"
                       : "+d"(cc)
                       : "a"(schib), "d"(r1), "m"(schib)
                       : "cc", "memory");
  return cc >> 28;
}

int css_store_channel(struct css_schid schid, struct css_schib *schib) {
  register struct css_schid r1 __asm__("1") = schid;
  int cc = -1;
  __asm__ __volatile__("stsch 0(%2)\n"
                       "ipm %0"
                       : "+d"(cc), "=m"(*schib)
                       : "a"(schib), "d"(r1)
                       : "cc", "memory");
  return cc >> 28;
}

int css_modify_channel(struct css_schid schid, void *schib) {
  register struct css_schid r1 __asm__("1") = schid;
  int cc = -1;
  __asm__ __volatile__("msch 0(%1)\n"
                       "ipm %0"
                       : "+d"(cc)
                       : "a"(schib), "d"(r1), "m"(schib)
                       : "cc", "memory");
  return cc >> 28;
}

int css_test_channel(struct css_schid schid, struct css_irb *schib) {
  register struct css_schid r1 __asm__("1") = schid;
  int cc = -1;
  __asm__ __volatile__("tsch 0(%2)\n"
                       "ipm %0"
                       : "+d"(cc), "=m"(*schib)
                       : "a"(schib), "d"(r1)
                       : "cc", "memory");
  return cc >> 28;
}


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
  static struct s390_psw rb_newio = {0x000C0000, AMBIT + (uint32_t) && rb_count};
  int r;

  read_block.bb = 0;
  read_block.cc = cyl;
  read_block.hh = head;
  read_block.record = rec;

  rb_cchain[3].addr = (uint32_t)buf;
  rb_cchain[3].length = (uint16_t)n;

  *((volatile uint64_t *)FLCINPSW) = *((uint64_t *)&rb_newio);
  __asm__ __volatile__("stosm 0x78, 0x00");
  *((volatile uint32_t *)FLCCAW) = (uint32_t)&rb_cchain[0];
  r = css_test_channel(schid, &rb_irb);
  if(r != 0) {
    //diag8_write("Test channel error", 19);
    //return -1;
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

__attribute__((aligned(4))) static uint32_t all_io_int = 0xFF000000;
extern unsigned char disk_buffer[];
static struct css_schid schid;

struct fdscb {
  uint16_t cyl;
  uint16_t head;
  uint8_t rec;
};

int find_file(struct fdscb *out_dscb, const char *name) {
  struct {
    char ds1dsnam[44];
    char ds1fmtid;
    char unused1[60];
    char unused2;
    char unused3;
    uint16_t start_cc;
    uint16_t start_hh;
    uint16_t end_cc;
    uint16_t end_hh;
  } dscb1;
  int cyl, head, rec;
  int r = 0, errcnt = 0;
  struct fdscb fdscb;

  r = read_disk(schid, 0, 0, 3, &disk_buffer, 32767);
  if(r >= 20) {
    memcpy(&fdscb.cyl, (char *)&disk_buffer + 15, 2); /* 15-16 */
    memcpy(&fdscb.head, (char *)&disk_buffer + 17, 2); /* 17-18 */
    memcpy(&fdscb.rec, (char *)&disk_buffer + 19, 1); /* 19-19 */

    cyl = fdscb.cyl;
    head = fdscb.head;
    rec = fdscb.rec;

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
          kprintf("DSCB CYL=%zu,HEAD=%zu,R=%zu %s\n", (size_t)cyl, (size_t)head, (size_t)rec, &dscb1.ds1dsnam);
          if(!memcmp(&dscb1.ds1dsnam, name, strlen(name))) {
            out_dscb->cyl = cyl;
            out_dscb->head = head;
            out_dscb->rec = rec;
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
    kprintf("File not found\n");
    return -1;
  }
  kprintf("File found\n");
  return 0;
}

static struct stivale2_struct st2_boot_cfg = {
  .bootloader_brand = "ENTERPRISE SYSTEM ARCHITECTURE 360, 370 AND 390 BOOTLOADER",
  .bootloader_version = "FRAMEBOOT VERSION 1.0, PRE-ALPHA",
  .tags = 0,
};

int main(void) {
  struct fdscb fdscb;
  signed int r;

  schid.id = 1;
  schid.num = 1;
  kprintf("s390 bootloader\n");

  read_disk(schid, 0, 0, 3, &disk_buffer, 4096);
  
  kprintf("Loading limine.cfg\n");
  r = find_file(&fdscb, "LIMINE.CFG");
  if(r != 0) {
    kprintf("No limine.cfg found\n");
    r = find_file(&fdscb, "FRAMEBOOT.CFG");
    if(r != 0) {
      kprintf("No frameboot.cfg found\n");
      while(1);
    }
  }

  /* TODO: Read limine.cfg */
  r = find_file(&fdscb, "KERNEL.BIN");
  if(r != 0) {
    kprintf("No kernel\n");
    while(1);
  }

  /* Get size of record */
  r = read_disk(schid, fdscb.cyl, fdscb.head, fdscb.rec, &disk_buffer, 32767);
  kprintf("Kernel has a size of %zu\n", (size_t)r);

  while (1)
    ;
}