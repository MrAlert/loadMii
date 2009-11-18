#ifndef __CACHE_H
#define __CACHE_H

#include "types.h"

void sync_before_read (void *p, u32 len);
void sync_after_write (const void *p, u32 len);
void sync_before_exec (const void *p, u32 len);

#endif /* __CACHE_H */
