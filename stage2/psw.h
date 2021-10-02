#ifndef PSW_H
#define PSW_H

struct s390_psw {
    uint32_t lo;
    uint32_t hi;
} __attribute__((packed, aligned(8)));

#define AMBIT 0x80000000
#define FLCCAW 0x48
#define FLCSNPSW 0x60
#define FLCPNPSW 0x68
#define FLCINPSW 0x78
#define FLCMNPSW 0x70
#define FLCIOA 0xB8

#endif