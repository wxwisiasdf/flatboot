#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>
#include <stddef.h>

void to_ebcdic(char *str);

int diag8_write(const void *buf, size_t size);
void kflush(void);
int kprintf(const char *fmt, ...);
int kvprintf(const char *fmt, va_list args);

#endif