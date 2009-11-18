// loaderstub - Wii SD card ELF loader stub
//
// Based on savezelda
// Copyright 2008,2009 Segher Boessenkool
// Copyright 2008      Haxx Enterprises
// Copyright 2008      Hector Martin ("marcan")
// Copyright 2003,2004 Felix Domke
//
// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// See file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "debug.h"
#include "types.h"
#include "util.h"
#include "ios.h"
#include "sd.h"
#include "fat.h"
#include "elf.h"
#include "stm.h"

static void dsp_reset (void) {
	write16(0x0c00500a, read16(0x0c00500a) & ~0x01f8);
	write16(0x0c00500a, read16(0x0c00500a) | 0x0010);
	write16(0x0c005036, 0);
}

static int try_sd_load (void) {
	int err;

	err = sd_init();
	if (err) {
		dprintf("SD card not found (%d)\n", err);
		return err;
	}

	err = fat_init();
	if (err == 0) dprintf("SD card detected\n");
	else {
		dprintf("SD card not detected (%d)\n", err);
		return err;
	}

	dprintf("Opening boot.elf...\n");
	err = fat_open("BOOT    ELF");

	if (err) {
		dprintf("boot.elf not found (%d)\n", err);
		return err;
	}

	dprintf("Reading %d bytes...\n", fat_file_size);
	err = fat_read(__code_buffer, fat_file_size);
	if (err) {
		dprintf("Error reading file (%d)\n", err);
		return err;
	}

	dprintf("Done.\n");
	return 0;
}

int main (void) {
	dsp_reset();

	debug_init();

	dprintf("loaderstub start\n");

	dprintf("Cleaning up environment...\n");
	reset_ios();
	dprintf("OK.\n");

	if (try_sd_load()) {
		dprintf("No code found. Resetting Wii...\n");
		int err;
		err = stm_hotreset();
		dprintf("Error resetting Wii (%d); hanging.\n", err);
		for (;;);
	}

	if (valid_elf_image(__code_buffer)) {
		dprintf("Valid ELF image detected.\n");
		void (*entry)() = load_elf_image(__code_buffer);
		stm_shutdown();
		entry();
		dprintf("Program returned to loader; restarting...\n");
		((void (*)())0x80001800)();
	}

	return 0;
}
