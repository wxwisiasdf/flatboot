AC_INIT([flatboot], [1.0], [jesusantonio30122016@gmail.com])
LDFLAGS="-Wl,-static,--no-dynamic-linker,-ztext -nostdlib -z max-page-size=0x1000 -static -pie"

AC_SUBST([LDFLAGS])
CFLAGS="$CFLAGS -ffreestanding -fno-stack-protector -pipe"
CFLAGS="$CFLAGS -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations -Wdouble-promotion -Wredundant-decls -Wnested-externs -Winline -Wconversion"
CFLAGS="$CFLAGS -std=gnu99 -m31 -fexec-charset=IBM-1047 -O2 -g"
ASFLAGS="$ASFLAGS -fexec-charset=IBM-1047 -g"

AM_INIT_AUTOMAKE([-Wall -Wextra foreign subdir-objects])

AM_PROG_AS
AC_PROG_CC

AC_CHECK_HEADERS([stdint.h stddef.h stdarg.h])

AM_CONDITIONAL(IS_X86_64, [test "${TARGET}" == "x86_64"])
AM_CONDITIONAL(IS_S390X, [test "${TARGET}" == "s390x"])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile stage1/Makefile stage2/Makefile barebones/Makefile])
AC_OUTPUT
