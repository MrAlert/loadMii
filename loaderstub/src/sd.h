#ifndef __SD_H
#define __SD_H

#include "types.h"

int sd_init (void);
int sd_read_sector (u8 *data, u32 offset);

#endif /* __SD_H */
