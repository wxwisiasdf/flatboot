#include <printf.h>
#include <string.h>
#include <stddef.h>

static const unsigned char ebc2asc[256] = {
    0x00, 0x01, 0x02, 0x03, 0x37, 0x2d, 0x2e, 0x2f, 0x16, 0x05, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x3c, 0x3d, 0x32, 0x26,
    0x18, 0x19, 0x3f, 0x27, 0x22, 0x1d, 0x35, 0x1f, 0x40, 0x5a, 0x7f, 0x7b,
    0x5b, 0x6c, 0x50, 0x7d, 0x4d, 0x5d, 0x5c, 0x4e, 0x6b, 0x60, 0x4b, 0x61,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0x7a, 0x5e,
    0x4c, 0x7e, 0x6e, 0x6f, 0x7c, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xad, 0xe0, 0xbd, 0x5f, 0x6d,
    0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x91, 0x92,
    0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,
    0xa7, 0xa8, 0xa9, 0xc0, 0x6a, 0xd0, 0xa1, 0x07, 0x9f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x41, 0xaa, 0x4a, 0xb1, 0x00, 0xb2, 0x6a, 0xb5,
    0xbd, 0xb4, 0x9a, 0x8a, 0x5f, 0xca, 0xaf, 0xbc, 0x90, 0x8f, 0xea, 0xfa,
    0xbe, 0xa0, 0xb6, 0xb3, 0x9d, 0xda, 0x9b, 0x8b, 0xb7, 0xb8, 0xb9, 0xab,
    0x64, 0x65, 0x62, 0x66, 0x63, 0x67, 0x9e, 0x68, 0x74, 0x71, 0x72, 0x73,
    0x78, 0x75, 0x76, 0x77, 0x44, 0x45, 0x42, 0x46, 0x43, 0x47, 0x9c, 0x48,
    0x54, 0x51, 0x52, 0x53, 0x58, 0x55, 0x56, 0x57, 0x8c, 0x49, 0xcd, 0xce,
    0xcb, 0xcf, 0xcc, 0xe1, 0x70, 0xdd, 0xde, 0xdb, 0xdc, 0x8d, 0x8e, 0x8f,
};

void to_ebcdic(
    char *str)
{
    size_t i, j;
    
    for(i = 0; i < strlen(str); i++) {
        size_t j;

        if(str[i] < 0x20 || str[i] > 0x80) {
            continue;
        }

        for(j = 0; j < 256; j++) {
            if(ebc2asc[j] == str[i]) {
                str[i] = ebc2asc[str[i]];
                break;
            }
        }
    }
}

static char numbuf[32], tmpbuf[160] = {0};
char *out_ptr = (char *)&tmpbuf[6];
int diag8_write(
    const void *buf,
    size_t size)
{
    size_t i;
    memcpy(&tmpbuf[0], "MSG * ", 6);
    for(i = 6; i < size + 6; i++) {
        if(tmpbuf[i] == '\n') {
            tmpbuf[i] = ' ';
        }

        /* Make it all uppercase just because we can */
        if(tmpbuf[i] >= 'a' && tmpbuf[i] <= 'i') {
            tmpbuf[i] = 'A' + (tmpbuf[i] - 'a');
        } else if(tmpbuf[i] >= 'j' && tmpbuf[i] <= 'r') {
            tmpbuf[i] = 'J' + (tmpbuf[i] - 'j');
        } else if(tmpbuf[i] >= 's' && tmpbuf[i] <= 'z') {
            tmpbuf[i] = 'S' + (tmpbuf[i] - 's');
        }
    }

    __asm__ __volatile__(
        "diag %0, %1, 8"
        :
        : "r"(&tmpbuf[0]), "r"(size + 5)
        : "cc", "memory");

    for(i = 0; i < sizeof(tmpbuf); i++) {
        tmpbuf[i] = ' ';
    }
    return 0;
}

void kflush(void) {
    *(out_ptr++) = '\0';
    out_ptr = (char *)&tmpbuf[6];
    diag8_write(out_ptr, strlen(out_ptr));
    return;
}

static inline void print_number(unsigned long val, int base) {
    char *buf_ptr = (char *)&numbuf;
    if(!val) {
        *(buf_ptr++) = '0';
    } else {
        while(val) {
            char rem = (char)(val % (unsigned long)base);
            *(buf_ptr++) = (rem >= 10) ? rem - 10 + 'A' : rem + '0';
            val /= (unsigned long)base;
        }
    }
    while(buf_ptr != (char *)&numbuf) {
        *(out_ptr++) = *(--buf_ptr);
    }
    return;
}

static inline void print_inumber(signed long val, int base) {
    char *buf_ptr = (char *)&numbuf;
    if(!val) {
        *(buf_ptr++) = '0';
    } else {
        while(val) {
            char rem = (char)(val % (long)base);
            *(buf_ptr++) = (rem >= 10) ? rem - 10 + 'A' : rem + '0';
            val /= base;
        }
    }

    if(val < 0) {
        *(out_ptr++) = '-';
    }
    while(buf_ptr != (char *)&numbuf) {
        *(out_ptr++) = *(--buf_ptr);
    }
    return;
}

int kprintf(
    const char *fmt,
    ...)
{
    va_list args;
    int r;

    va_start(args, fmt);
    r = kvprintf(fmt, args);
    va_end(args);
    return r;
}

int kvprintf(
    const char *fmt,
    va_list args)
{
    size_t i;

    while(*fmt != '\0') {
        if(*fmt == '\n') {
            *(out_ptr++) = *(fmt++);
            kflush();
            continue;
        }

        if(*fmt == '%') {
            ++fmt;
            if(!strncmp(fmt, "e", 1)) {
                const char *str = va_arg(args, const char *);
                if(str == NULL) {
                    kprintf("(nil)");
                } else {
                    for(i = 0; i < strlen(str); i++) {
                        size_t j;

                        if(str[i] < 0x20 || str[i] > 0x80) {
                            continue;
                        }

                        for(j = 0; j < 256; j++) {
                            if(ebc2asc[j] == str[i]) {
                                *(out_ptr++) = ebc2asc[str[i]];
                                break;
                            }
                        }
                    }
                }
            } else if(!strncmp(fmt, "s", 1)) {
                const char *str = va_arg(args, const char *);
                if(str == NULL) {
                    kprintf("(nil)");
                } else {
                    for(i = 0; i < strlen(str); i++) {
                        *(out_ptr++) = str[i];
                    }
                }
            } else if(!strncmp(fmt, "zu", 2)) {
                size_t val = va_arg(args, size_t);
                print_number((unsigned long)val, 10);
                ++fmt;
            } else if(!strncmp(fmt, "u", 1)) {
                unsigned int val = va_arg(args, unsigned int);
                print_number((unsigned long)val, 10);
            } else if(!strncmp(fmt, "p", 1)) {
                unsigned val = va_arg(args, unsigned);
                kprintf("'X");
                print_number((unsigned long)val, 16);
            } else if(!strncmp(fmt, "x", 1)) {
                unsigned val = va_arg(args, unsigned);
                print_number((unsigned long)val, 16);
            } else if(!strncmp(fmt, "i", 1)) {
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