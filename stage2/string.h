#ifndef STRING_H
#define STRING_H

#include <stddef.h>

static inline void *memcpy(void *dest, const void *src, size_t n) {
    const char *c_src = (const char *)src;
    char *c_dest = (char *)dest;
    while(n) {
        *(c_dest++) = *(c_src++);
        --n;
    }
    return dest;
}

static inline int memcmp(const void *s1, const void *s2, size_t n) {
    int diff = 0;
    const char *_s1 = (const char *)s1;
    const char *_s2 = (const char *)s2;

    while(n) {
        diff += *(_s1++) - *(_s2++);
        --n;
    }
    return diff;
}

static inline size_t strlen(const char *s) {
    size_t i = 0;
    while(*s != '\0') {
        ++i;
        ++s;
    }
    return i;
}

static inline int strcmp(const char *s1, const char *s2) {
    size_t n = strlen(s1);
    int diff = 0;

    while(n && *s1 != '\0' && *s2 != '\0') {
        diff += *(s1++) - *(s2++);
        --n;
    }

    if(*s1 != *s2) {
        return -1;
    }
    return diff;
}

static inline int strncmp(const char *s1, const char *s2, size_t n) {
    int diff = 0;

    while(n && *s1 != '\0' && *s2 != '\0') {
        diff += *(s1++) - *(s2++);
        --n;
    }

    if(*s1 != *s2 && n > 0) {
        return -1;
    }
    return diff;
}

#endif