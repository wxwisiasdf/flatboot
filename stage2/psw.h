#ifndef PSW_H
#define PSW_H

struct s390_psw {
    uint32_t flags;
    uint32_t address;
} __attribute__((packed, aligned(8)));

struct s390x_psw {
    uint32_t hi_flags;
    uint32_t lo_flags; /* It's all zero except for the MSB (in S/390 order) */
    uint32_t hi_address;
    uint32_t lo_address;
} __attribute__((packed, aligned(8)));

#define AMBIT 0x80000000
#define AM64 0x00000001

#define FLCCAW 0x48
#define FLCSNPSW 0x60
#define FLCPNPSW 0x68
#define FLCINPSW 0x78
#define FLCMNPSW 0x70
#define FLCIOA 0xB8
#define FLCEINPSW 0x1F0

#endif