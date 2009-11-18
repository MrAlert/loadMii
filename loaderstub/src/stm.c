// Copyright 2009 Alan Williams <mralert@gmail.com>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

// Based on code:
//	Copyright (C) 2008 Michael Widenbaur (shagkur)
//	Copyright (C) 2008 Dave Murphy (WinterMute)
//	Copyright (C) 2008 Hector Martin (marcan)

#include "types.h"
#include "string.h"
#include "ios.h"
#include "stm.h"

#define STM_HOTRESET   0x2001
#define STM_RELEASE_EH 0x3002

static int stm_imm_fd = -1;

static u32 stm_inbuf[8];
static u32 stm_outbuf[8];

int stm_init (void) {
	stm_imm_fd = ios_open("/dev/stm/immediate", IOS_OPEN_NONE);
	return stm_imm_fd;
}

int stm_shutdown (void) {
	int ret = ios_close(stm_imm_fd);
	stm_imm_fd = -1;
	return ret;
}

int stm_releaseeventhook (void) {
	return ios_ioctl(stm_imm_fd, STM_RELEASE_EH, stm_inbuf, sizeof(stm_inbuf), stm_outbuf, sizeof(stm_outbuf));
}

int stm_hotreset (void) {
#ifdef DEBUG
	return -1;
#else
	stm_inbuf[0] = 0;
	return ios_ioctl(stm_imm_fd, STM_HOTRESET, stm_inbuf, sizeof(stm_inbuf), stm_outbuf, sizeof(stm_outbuf));
#endif
}
