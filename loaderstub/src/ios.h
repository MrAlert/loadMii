#ifndef __IOS_H
#define __IOS_H

#include "types.h"

#define IOS_OPEN_NONE  0
#define IOS_OPEN_READ  1
#define IOS_OPEN_WRITE 2
#define IOS_OPEN_RW    (IOS_OPEN_READ | IOS_OPEN_WRITE)

#define IOS_SEEK_SET 0
#define IOS_SEEK_CUR 1
#define IOS_SEEK_END 2

struct ioctlv {
	void *data;
	u32 len;
};

int ios_open (const char *filename, u32 mode);
int ios_close (int fd);
int ios_read (int fd, void *data, u32 len);
int ios_write (int fd, const void *data, u32 len);
int ios_seek (int fd, int where, int whence);
int ios_ioctl (int fd, u32 n, const void *in, u32 inlen, void *out, u32 outlen);
int ios_ioctlv (int fd, u32 n, u32 in_count, u32 out_count, struct ioctlv *vec);

void reset_ios (void);

#endif /* __IOS_H */
