// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "debug.h"
#include "types.h"
#include "string.h"
#include "util.h"
#include "cache.h"
#include "time.h"
#include "ios.h"
#include "stm.h"

// Low-level IPC access.

static u32 ipc_read (u32 reg) {
	return read32(0x0d000000 + 4 * reg);
}

static void ipc_write (u32 reg, u32 value) {
	write32(0x0d000000 + 4 * reg, value);
}

static void ipc_bell (u32 w) {
	ipc_write(1, (ipc_read(1) & 0x30) | w);
}

static void ipc_wait_ack (void) {
	while ((ipc_read(1) & 0x22) != 0x22);
}

static void ipc_wait_reply (void) {
	while ((ipc_read(1) & 0x14) != 0x14);
}

static void ipc_irq_ack (void) {
	ipc_write(12, 0x40000000);
}

// Mid-level IPC access.

static struct {
	u32 cmd;
	int result;
	int fd;
	u32 arg[5];
	u32 user[8];
} ipc __attribute__((aligned(64)));

static void ipc_send_request (void) {
	sync_after_write(&ipc, 0x40);

	ipc_write(0, virt_to_phys(&ipc));
	ipc_bell(1);

	ipc_wait_ack();

	ipc_bell(2);
	ipc_irq_ack();
}

static void ipc_recv_reply (void) {
	for (;;) {
		u32 reply;

		ipc_wait_reply();

		reply = ipc_read(2);
		ipc_bell(4);

		ipc_irq_ack();
		ipc_bell(8);

		if (reply == virt_to_phys(&ipc)) break;
	}

	sync_before_read(&ipc, sizeof(ipc));
}

// High-level IPC access.

#define IOS_OPEN   1
#define IOS_CLOSE  2
#define IOS_READ   3
#define IOS_WRITE  4
#define IOS_SEEK   5
#define IOS_IOCTL  6
#define IOS_IOCTLV 7

int ios_open (const char *filename, u32 mode) {
	sync_after_write(filename, strlen(filename) + 1);

	memset(&ipc, 0, sizeof(ipc));
	ipc.cmd = IOS_OPEN;
	ipc.fd = 0;
	ipc.arg[0] = virt_to_phys(filename);
	ipc.arg[1] = mode;

	ipc_send_request();
	ipc_recv_reply();

	return ipc.result;
}

int ios_close (int fd) {
	memset(&ipc, 0, sizeof(ipc));
	ipc.cmd = IOS_CLOSE;
	ipc.fd = fd;

	ipc_send_request();
	ipc_recv_reply();

	return ipc.result;
}

#if 0
int ios_read (int fd, void *data, u32 len) {
	memset(&ipc, 0, sizeof(ipc));
	ipc.cmd = IOS_READ;
	ipc.fd = fd;
	ipc.arg[0] = virt_to_phys(data);
	ipc.arg[1] = len;

	ipc_send_request();
	ipc_recv_reply();

	if (data) sync_before_read(data, len);

	return ipc.result;
}

int ios_write (int fd, const void *data, u32 len) {
	if (data) sync_after_write(data, len);

	memset(&ipc, 0, sizeof(ipc));
	ipc.cmd = IOS_WRITE;
	ipc.fd = fd;
	ipc.arg[0] = virt_to_phys(data);
	ipc.arg[1] = len;

	ipc_send_request();
	ipc_recv_reply();

	return ipc.result;
}

int ios_seek (int fd, int where, int whence) {
	memset(&ipc, 0, sizeof(ipc));
	ipc.cmd = IOS_SEEK;
	ipc.fd = fd;
	ipc.arg[0] = where;
	ipc.arg[1] = whence;

	ipc_send_request();
	ipc_recv_reply();

	return ipc.result;
}
#endif

int ios_ioctl (int fd, u32 n, const void *in, u32 inlen, void *out, u32 outlen) {
	if (in) sync_after_write(in, inlen);
	if (out) sync_after_write(out, outlen);

	memset(&ipc, 0, sizeof(ipc));
	ipc.cmd = IOS_IOCTL;
	ipc.fd = fd;
	ipc.arg[0] = n;
	ipc.arg[1] = virt_to_phys(in);
	ipc.arg[2] = inlen;
	ipc.arg[3] = virt_to_phys(out);
	ipc.arg[4] = outlen;

	ipc_send_request();
	ipc_recv_reply();

	if (out) sync_before_read(out, outlen);

	return ipc.result;
}

int ios_ioctlv (int fd, u32 n, u32 in_count, u32 out_count, struct ioctlv *vec) {
	u32 i;

	for (i = 0; i < in_count + out_count; i++)
		if (vec[i].data) {
			sync_after_write(vec[i].data, vec[i].len);
			vec[i].data = (void *)virt_to_phys(vec[i].data);
		}

	sync_after_write(vec, (in_count + out_count) * sizeof(*vec));

	memset(&ipc, 0, sizeof(ipc));
	ipc.cmd = IOS_IOCTLV;
	ipc.fd = fd;
	ipc.arg[0] = n;
	ipc.arg[1] = in_count;
	ipc.arg[2] = out_count;
	ipc.arg[3] = virt_to_phys(vec);

	ipc_send_request();
	ipc_recv_reply();

	for (i = in_count; i < in_count + out_count; i++)
		if (vec[i].data) {
			vec[i].data = phys_to_virt((u32)vec[i].data);
			sync_before_read(vec[i].data, vec[i].len);
		}

	return ipc.result;
}

// Cleanup any old state.

static void ipc_cleanup_request (void) {
	if ((ipc_read(1) & 0x22) == 0x22)
		ipc_bell(2);
}

static void ipc_cleanup_reply (void) {
	if ((ipc_read(1) & 0x14) != 0x14) return;

	ipc_read(2);
	ipc_bell(4);

	ipc_irq_ack();
	ipc_bell(8);
}

static void release_old_stm_callback (void) {
	int ret;
	*((u32 *)0x80000018) = 0x00000014;
	sync_after_write((void *)0x80000014, 8);

	ret = stm_init();
	if (ret < 0) {
		dprintf("STM initialization failed!\n");
		return;
	}

	ret = stm_releaseeventhook();
	if (ret < 0) dprintf("Eventhook release failed with code %d\n", ret);
}

void reset_ios (void) {
	int i;

	dprintf("Flusing IPC transactions");
	for (i = 0; i < 10; i++) {
		ipc_cleanup_request();
		ipc_cleanup_reply();
		ipc_irq_ack();
		udelay(1000);
		dprintf(".");
	}
	dprintf(" Done.\n");

	dprintf("Closing file descriptors...");
	for (i = 0; i < 32; i++) ios_close(i);
	dprintf(" Done.\n");

	release_old_stm_callback();
}
