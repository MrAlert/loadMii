// Copyright 2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// See file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "debug.h"
#include "types.h"
#include "util.h"
#include "fb.h"

static int debug_inited = 0;

static int dputc (int c) {
	return fb_putc(c);
}

void debug_init (void) {
	/* Clear interrupt mask. */
	write32(0x0c003004, 0);

	/* Unlock EXI. */
	write32(0x0d00643c, 0);

	fb_init();

	debug_inited = 1;
}

void dprintf (const char *fmt, ...) {
	if (!debug_inited) return;
	while (*fmt) {
		if (*fmt != '%') {
			dputc(*fmt++);
			continue;
		}
		fmt++;
	}
}
