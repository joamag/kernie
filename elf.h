#ifndef ELF_H
#define ELF_H

#include <stdint.h>

typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) Elf64_Phdr;

#define PT_LOAD    1
#define ELF_MAGIC  0x464C457F

typedef void (*entry_fn)(void);

static void memcpy_k(void *dst, const void *src, uint64_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
}

static void memset_k(void *dst, uint8_t val, uint64_t n) {
    uint8_t *d = dst;
    while (n--) *d++ = val;
}

static int load_elf(void *elf_data) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf_data;

    if (*(uint32_t *)ehdr->e_ident != ELF_MAGIC)
        return -1;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)elf_data + ehdr->e_phoff + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD)
            continue;

        memcpy_k((void *)phdr->p_vaddr,
                 (uint8_t *)elf_data + phdr->p_offset,
                 phdr->p_filesz);

        if (phdr->p_memsz > phdr->p_filesz) {
            memset_k((void *)(phdr->p_vaddr + phdr->p_filesz),
                     0,
                     phdr->p_memsz - phdr->p_filesz);
        }
    }

    entry_fn entry = (entry_fn)ehdr->e_entry;
    entry();

    return 0;
}

#endif
