#ifndef __FONT_H
#define __FONT_H

#include "types.h"

#define FONT_WIDTH 12
#define FONT_HEIGHT 12

typedef u8 glyph[FONT_WIDTH * FONT_HEIGHT / 2];

extern glyph font_glyphs[];
extern u8 font_widths[];

void font_init();

#endif
