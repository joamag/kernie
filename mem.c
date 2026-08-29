#include "mem.h"

/* GCC emits calls to these four even under -ffreestanding, for struct
   assignments and any copy it decides not to inline, so the kernel has to
   provide them itself */

void *memcpy(void *dst, const void *src, uint64_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, uint64_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;

    /* copy backwards when the regions overlap the wrong way */
    if (d > s) {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    } else {
        while (n--) *d++ = *s++;
    }

    return dst;
}

void *memset(void *dst, int val, uint64_t n) {
    uint8_t *d = dst;
    while (n--) *d++ = (uint8_t)val;
    return dst;
}

int memcmp(const void *a, const void *b, uint64_t n) {
    const uint8_t *x = a;
    const uint8_t *y = b;

    while (n--) {
        if (*x != *y)
            return *x - *y;
        x++;
        y++;
    }

    return 0;
}
