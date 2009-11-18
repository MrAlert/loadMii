// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Copyright 2003-2004  Felix Domke <tmbinc@elitedvb.net>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include "util.h"
#include "cache.h"
#include "font.h"
#include "fb.h"

#define TAB_SIZE 96

static struct {
	u32 xres, yres, stride;
	u32 cursor_x, cursor_y;
	u32 border_left, border_right, border_top, border_bottom;
} fb;

static void fb_write (u32 offset, u32 x) {
	u32 *p = (u32 *)(0x80f00000 + offset);
	*p = x;
	sync_after_write(p, 4);
}

static u32 fb_read (u32 offset) {
	u32 *p = (u32 *)(0x80f00000 + offset);
	return *p;
}

static void fb_clear_lines (u32 top, u32 lines) {
	u32 x, y;
	u32 offset;

	offset = fb.stride * top;

	for (y = 0; y < lines; y++) {
		for (x = 0; x < fb.xres / 2; x++)
			fb_write(offset + 4 * x, 0x00800080);
		offset += fb.stride;
	}
}

static void fb_scroll_line (void) {
	u32 x, y;
	u32 offset, delta;
	u32 lines = FONT_HEIGHT;

	offset = fb.stride * fb.border_top;
	delta = fb.stride * lines;

	for (y = fb.border_top; y < fb.yres - lines; y++) {
		for (x = 0; x < fb.xres / 2; x++)
			fb_write(offset + 4 * x, fb_read(offset + 4 * x + delta));
		offset += fb.stride;
	}

	fb_clear_lines(fb.yres - lines, lines);

	fb.cursor_y -= lines;
}

static u32 fb_drawc (u32 x, u32 y, u8 c) {
	if (c == ' ') return FONT_WIDTH;
	u32 offset = fb.stride * y + 2 * x;
	u32 ax, ay;
	for (ay = 0; ay < FONT_HEIGHT; ay++) {
		for (ax = 0; ax < FONT_WIDTH / 2; ax++)
			fb_write(offset + 4 * ax, 0x03fe7bcd);
		offset += fb.stride;
	}

	return FONT_WIDTH;
}

int fb_putc (int c) {
	u32 tabpos;
	unsigned char n = c;
	switch (n) {
		case '\n':
			fb.cursor_y += FONT_HEIGHT;
		case '\r':
			fb.cursor_x = fb.border_left;
			break;
		case '\t':
			tabpos = (fb.cursor_x - fb.border_left) / TAB_SIZE;
			fb.cursor_x = fb.border_left + (tabpos + 1) * TAB_SIZE;
			if ((fb.cursor_x + FONT_WIDTH) > fb.border_right) {
				fb.cursor_y += FONT_HEIGHT;
				fb.cursor_x = fb.border_left;
			}
			break;
		default:
			fb.cursor_x += fb_drawc(fb.cursor_x, fb.cursor_y, n);
			if ((fb.cursor_x + FONT_WIDTH) > fb.border_right) {
				fb.cursor_y += FONT_HEIGHT;
				fb.cursor_x = fb.border_left;
			}
	}

	while (fb.cursor_y + FONT_HEIGHT >= fb.border_bottom) fb_scroll_line();

	return n;
}

static void fb_size (u32 xres, u32 yres, u32 stride) {
	fb.xres = xres;
	fb.yres = yres;
	fb.stride = stride;
	fb.border_left = 30;
	fb.border_top = 30;
	fb.border_right = fb.xres - 30;
	fb.border_bottom = fb.yres - 30;

	fb.cursor_x =fb.border_left;
	fb.cursor_y = fb.border_top;

	fb_clear_lines(0, fb.yres);
}

void fb_init (void) {
	u32 vtr = read16(0x0c002000);
	u32 lines = vtr >> 4;
	if ((vtr & 0x0f) > 10) {
		write32(0x0c00201c, 0x00f00000);
		write32(0x0c002024, 0x00f00000);
	} else {
		lines *= 2;

		u32 vto = read32(0x0c00200c);
		u32 vte = read32(0x0c002010);

		if ((vto & 0x03ff) < (vte & 0x03ff)) {
			write32(0x0c00201c, 0x00f00000);
			write32(0x0c002024, 0x00f00000 + 2 * 640);
		} else {
			write32(0x0c00201c, 0x00f00000 + 2 * 640);
			write32(0x0c002024, 0x00f00000);
		}
	}

	fb_size(640, lines, 2 * 640);
	font_init();
}

