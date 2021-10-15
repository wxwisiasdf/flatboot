bin_PROGRAMS = kernel

kernel_SOURCES = kernel.c
kernel_CFLAGS = -fno-pic -fno-pie -O2 -mpacked-stack
kernel_LDFLAGS = -T$(top_srcdir)/barebones/linker.ld
