#ifndef MEM_H
#define MEM_H

#include <stdint.h>

void *memcpy(void *dst, const void *src, uint64_t n);
void *memmove(void *dst, const void *src, uint64_t n);
void *memset(void *dst, int val, uint64_t n);
int memcmp(const void *a, const void *b, uint64_t n);

#endif
