bin_PROGRAMS = stage1

stage1_SOURCES = boot.S

if ZARCH
    stage1_LDFLAGS = -T$(top_srcdir)/stage1/zarch.ld
else
    stage1_LDFLAGS = -T$(top_srcdir)/stage1/esa.ld
endif
