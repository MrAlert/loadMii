#ifndef __ELF_H
#define __ELF_H
#include "types.h"

int valid_elf_image (void *addr);
void *load_elf_image (void *addr);

#endif /* __ELF_H */
