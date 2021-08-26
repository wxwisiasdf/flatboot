#
# This is a gnu assembler re-implementation of PDOS's sapstart.asm
# with a special touch from me, of course ;)
#

.macro ccw0 cmd=0, addr=0, flags=0, length=0
    .byte \cmd
    .short (\addr & 0xffff)
    .byte ((\addr & 0xff0000) >> 16)
    .byte \flags
    .byte 0
    .short \length
.endm

.macro ccw1 cmd=0, addr=0, flags=0, length=0
    .byte \cmd
    .byte \flags
    .short \length
    .long \addr
.endm

.equ AMBIT, 0x80000000
.equ FLCCAW, 0x48
.equ FLCSNPSW, 0x60
.equ FLCPNPSW, 0x68
.equ FLCINPSW, 0x78
.equ FLCMNPSW, 0x70
.equ FLCIOA, 0xB8

.section .text.init
    # Initial PSW (tells the machine to jump to start)
    .long 0x000C0000
    .long (AMBIT + start)

.section .text
.global start
.type start, @function
start:
    mvc FLCINPSW(8), waiter1
    mvc FLCMNPSW(8), waiter2
    mvc FLCSNPSW(8), waiter3
    mvc FLCPNPSW(8), waiter4

    # Obtain the IPL device address and save it on r1 because store
    # subchannel uses r1 as device address register which uses the
    # higher half for ID and the lower half for the device number
    lhi %r1, 0
    icm %r1, 0b1111, FLCIOA
#ifdef __s390__
    lctl 6, 6, all_io_int
#endif
    lhi %r2, 0 # head=0, cyl=1, rec=1
    lhi %r3, 1
    lhi %r4, 1
    larl %r5, disk_buffer
    lhi %r6, 32767
    brasl %r14, read_block

    lhi %r2, 0 # head=0, cyl=1, rec=1
    lhi %r3, 1
    lhi %r4, 1
    larl %r5, disk_buffer
    lhi %r6, 32767
    brasl %r14, read_block

    j disk_buffer

# Read a block from the DASD
# r1 = Device number (subchannel id)
# r2 = Head
# r3 = Cylinder
# r4 = Record
# r5 = Buffer
# r6 = Size of buffer
read_block:
    # Store from stack
    stcm %r2, 0b0011, rb_cc1 # Cylinder
    stcm %r3, 0b0011, rb_hh1 # Head
    stc %r4, rb_record # Record

    # r5 = buffer, r6 = length
#ifdef __s390__
    st %r5, rb_ldccw + 4
    sth %r6, rb_ldccw + 2
#else
    stcm %r5, 0b0111, rb_ldccw + 1
    sth %r6, rb_ldccw + 6
#endif
    mvc FLCINPSW(8), rb_newio
    stosm FLCINPSW, 0x00
    la %r3, rb_seek
    st %r3, FLCCAW
#ifdef __s390__
    la %r3, rb_irb
    tsch 0(%r3)
#    ipm %r2
#    srl %r2, 28
#    bnz fatal_error

    la %r3, rb_orb
    ssch 0(%r3)
#    ipm %r2
#    srl %r2, 28
#    bnz fatal_error
#else
    sio 0(%r1)
#endif
    lpsw rb_wtnoer

# New IO code will jump here
.align 4
rb_count:
#ifdef __s390__
    la %r3, rb_irb
    tsch 0(%r3)
#    ipm %r2
#    srl %r2, 28
#    bnz fatal_error
    clc 4(4, %r3), rb_end_chain
#else
    clc FLCCSW(4), rb_end_chain
#endif
rb_end:
    br %r14

# Reports fatal error then hangs
fatal_error:
    larl %r0, .err_msg
    lhi %r1, 16
    diag %r0, %r1, 8
.short 0

.align 8
#if defined(__s390__)
rb_seek:
    ccw1 7, rb_bbcchh, 0x40, 6
rb_search:
    ccw1 0x31, rb_cchhr, 0x40, 5
    ccw1 8, rb_search, 0, 0
rb_ldccw:
    ccw1 0x0e, 0, 0x20, 32767
#else
rb_seek:
    ccw0 7, rb_bbcchh, 0x40, 6
rb_search:
    ccw0 0x31, rb_cchhr, 0x40, 5
    ccw0 8, rb_search, 0, 0
rb_ldccw:
    ccw0 0x0e, 0, 0x20, 32767
#endif
rb_end_chain:

.align 2
rb_bbcchh:
    .short 0
rb_cchhr:
rb_cc1:
    .short 0 # 2 uninit bytes
rb_hh1:
    .short 0 # 2 uninit bytes
rb_record:
    .byte 0

.align 8
rb_wtnoer:
    .long 0x060E0000
    .long AMBIT
rb_newio:
    .long 0x000C0000
    .long (AMBIT + rb_count)

waiter1:
    .long 0x000E0000
    .long (AMBIT + 0x00000111)
waiter2:
    .long 0x000E0000
    .long (AMBIT + 0x00000222)
waiter3:
    .long 0x000E0000
    .long (AMBIT + 0x00000333)
waiter4:
    .long 0x000E0000
    .long (AMBIT + 0x00000444)

.align 4
all_io_int:
    .long 0xFF000000

.align 4
rb_orb:
    .long 0
    .long 0x0080FF00
    .long rb_seek
    .long 0 # 5, 4 bytes of 0
    .long 0
    .long 0
    .long 0
    .long 0

.align 4
rb_irb:
    .long 0 # 24, 4 bytes of 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0

# EBCDIC message of "MSG * Disk error"
.err_msg:
.ascii "\xD4\xE2\xC7\x40\x5C\x40\xC4\x89\xA2\x92\x40\x85\x99\x99\x96\x99"

.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
