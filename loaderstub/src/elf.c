// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include "string.h"
#include "cache.h"
#include "elf.h"

int valid_elf_image (void *addr) {
	u32 *header = addr;
	return header[0] == 0x7f454c46
		&& header[1] == 0x01020100
		&& header[4] == 0x00020014
		&& header[5] == 1
		&& (header[10] & 0xffff) == 32;	
}

void *load_elf_image (void *addr) {
	u32 *header = addr;
	u32 *phdr = addr + header[7];
	u32 n = header[11] >> 16;
	u32 i;

	for (i = 0; i < n; i++, phdr += 8) {
		if (phdr[0] != 1) continue;

		u32 off = phdr[1];
		void *dest = (void *)phdr[3];
		u32 filesz = phdr[4];
		u32 memsz = phdr[5];

		memcpy(dest, addr + off, filesz);
		memset(dest + filesz, 0, memsz - filesz);

		sync_before_exec(dest, memsz);
	}

	return (void *)header[6];
}
