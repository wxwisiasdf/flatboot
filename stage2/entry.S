.section .text.init

.global start
.type start, @function
start:
    # Set the stack pointer
    larl %r15, stack_top

    # Clear bss
    lhi %r0, 0
    larl %r1, bss_start
    larl %r2, bss_end
.loop:
    stc %r0, %r1
    ahi %r1, 1
    cr %r1, %r2
    brne .loop

    brasl %r14, main
.short 0