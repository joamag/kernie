/**
 * lib/mem.c
 *
 * Freestanding memcpy, memmove, memset and memcmp.
 *
 * These are not a convenience. The C standard lets an implementation emit
 * calls to all four even under -ffreestanding, and GCC does so for struct
 * assignments, large local array initialisers and any copy it decides not to
 * inline. Without them the kernel fails to link, and the error points at the
 * line that triggered the call rather than at anything obviously wrong.
 *
 * The loops are deliberately naive byte copies. Anything cleverer would want
 * alignment handling that the arm64 target cannot rely on while its MMU is
 * off, and the volumes involved here do not justify it. The build also passes
 * -fno-tree-loop-distribute-patterns, without which GCC recognises the shape
 * of these very loops and rewrites them into calls to themselves.
 *
 * memmove compares integer representations rather than the pointers
 * themselves, because relational comparison of pointers into distinct objects
 * is undefined, and distinct objects are exactly what its contract allows.
 */

#include "lib/mem.h"

void *memcpy(void *dst, const void *src, uint64_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, uint64_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;

    // copy backwards when the regions overlap the wrong way
    if ((uintptr_t)d > (uintptr_t)s) {
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
