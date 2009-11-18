// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include "time.h"

static u32 mftb (void) {
	u32 x;
	asm volatile("mftb %0" : "=r"(x));
	return x;
}

static void __delay (u32 ticks) {
	u32 start = mftb();
	while (mftb() - start < ticks);
}

void udelay (u32 us) {
	__delay(us * 243 / 4);
}
