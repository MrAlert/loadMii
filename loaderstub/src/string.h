#ifndef __STRING_H
#define __STRING_H
#include "types.h"

int memcmp (const void *s1, const void *s2, size_t n);
void *memcpy (void *dest, const void *src, size_t n);
void *memset (void *s, int c, size_t n);
size_t strlen (const char *s);

#endif /* __STRING_H */
