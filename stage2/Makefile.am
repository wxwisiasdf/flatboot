bin_PROGRAMS = stage2

stage2_SOURCES = entry.S stivale.c printf.c dasd.c load_kernel.c
stage2_CFLAGS = -fno-pic -fno-pie -O0 -mpacked-stack
stage2_LDFLAGS = -T$(top_srcdir)/stage2/linker.ld
