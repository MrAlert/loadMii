#ifndef __FAT_H
#define __FAT_H
#include "types.h"

int fat_init (void);
int fat_open (const char *filename);
int fat_read (void *dest, u32 size);

extern u32 fat_file_size;

#endif /* __FAT_H */
