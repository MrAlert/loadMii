// Copyright 2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// See file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include "string.h"
#include "util.h"
#include "sd.h"
#include "fat.h"

#define RAW_BUF 0x200
static u8 raw_buf[RAW_BUF] __attribute__((aligned(32)));

static int raw_read (u32 sector) {
	static u32 current = -1;
	if (current == sector) return 0;
	current = sector;
	return sd_read_sector(raw_buf, sector);
}

static u64 partition_start_offset;

static int read (u8 *data, u64 offset, u32 len) {
	offset += partition_start_offset;
	while (len) {
		u32 buf_off = offset % RAW_BUF;
		u32 n;

		n = RAW_BUF - buf_off;
		if (n > len) n = len;

		int err = raw_read(offset / RAW_BUF);
		if (err) return err;

		memcpy(data, raw_buf + buf_off, n);

		data += n;
		offset += n;
		len -= n;
	}

	return 0;
}

static u32 bytes_per_cluster;
static u32 root_entries;
static u32 clusters;
static u32 fat_type;

static u64 fat_offset;
static u64 root_offset;
static u64 data_offset;

static u32 get_fat (u32 cluster) {
	u8 fat[4];

	u32 offset_bits = cluster * fat_type;
	int err = read(fat, fat_offset + offset_bits/8, 4);
	if (err) return 0;

	u32 res = le32(fat) >> (offset_bits % 8);
	res &= (1 << fat_type) - 1;
	res &= 0x0fffffff;

	return res;
}

static u64 extent_offset;
static u32 extent_len;
static u32 extent_next_cluster;

static void get_extent (u32 cluster) {
	extent_len = 0;
	extent_next_cluster = 0;

	if (cluster == 0) {
		if (fat_type != 32) {
			extent_offset = root_offset;
			extent_len = 0x20 * root_entries;
			return;
		}
		cluster = root_offset;
	}

	if (cluster - 2 >= clusters) return;

	extent_offset = data_offset + (u64)bytes_per_cluster * (cluster - 2);

	for (;;) {
		extent_len += bytes_per_cluster;
		u32 next_cluster = get_fat(cluster);
		if (next_cluster - 2 >= clusters) break;
		if (next_cluster != cluster + 1) {
			extent_next_cluster = next_cluster;
			break;
		}

		cluster = next_cluster;
	}
}

static int read_extent (u8 *data, u32 len) {
	while (len) {
		if (extent_len == 0) return -1;

		u32 this = len;
		if (this > extent_len) this = extent_len;

		int err = read(data, extent_offset, this);
		if (err) return err;

		extent_offset += this;
		extent_len -= this;

		data += this;
		len -= this;

		if (extent_len == 0 && extent_next_cluster)
			get_extent(extent_next_cluster);
	}

	return 0;
}

int fat_read (void *data, u32 len) {
	return read_extent(data, len);
}

#if 0
static u8 fat_name[11];

static u8 ucase (char c) {
	if (c >= 'a' && c <= 'z') return c - 'a' + 'A';
	return c;
}

static const char *parse_component (const char *path) {
	u32 i = 0;

	while (*path == '/') path++;

	while (*path && *path != '/' && *path != '.') {
		if (i < 8) fat_name[i++] = ucase(*path);
		path++;
	}

	while (i < 8) fat_name[i++] = ' ';

	if (*path == '.') path++;

	while (*path && *path != '/') {
		if (i < 11) fat_name[i++] = ucase(*path);
		path++;
	}

	while (i < 11) fat_name[i++] = ' ';

	if (fat_name[0] == 0xe5) fat_name[0] = 0x05;

	return path;
}
#endif

u32 fat_file_size;

int fat_open (const char *name) {
	get_extent(0);

	while (extent_len) {
		u8 dir[0x20];

		int err = read_extent(dir, 0x20);
		if (err) return err;

		if (dir[0] == 0) return -1;

		if (dir[0x0b] & 0x08) continue;

		if (dir[0x00] == 0xe5) continue;

		if (dir[0x0b] & 0x10) continue;

		if (memcmp(name, dir, 11) == 0) {
			u32 cluster = le16(dir + 0x1a);
			if (fat_type == 32) cluster |= le16(dir + 0x14) << 16;

			fat_file_size = le32(dir + 0x1c);
			get_extent(cluster);

			return 0;
		}
	}

	return -1;
}

static int fat_init_fs (const u8 *sb) {
	u32 bytes_per_sector = le16(sb + 0x0b);
	u32 sectors_per_cluster = sb[0x0d];
	bytes_per_cluster = bytes_per_sector * sectors_per_cluster;

	u32 reserved_sectors = le16(sb + 0x0e);
	u32 fats = sb[0x10];
	root_entries = le16(sb + 0x11);
	u32 total_sectors = le16(sb + 0x13);
	u32 sectors_per_fat = le16(sb + 0x16);

	if (total_sectors == 0) total_sectors = le32(sb + 0x20);

	if (sectors_per_fat == 0) sectors_per_fat = le32(sb + 0x24);

	u32 fat_sectors = sectors_per_fat * fats;
	u32 root_sectors = (0x20 * root_entries + bytes_per_sector - 1)
		/ bytes_per_sector;
	u32 fat_start_sector = reserved_sectors;
	u32 root_start_sector = fat_start_sector + fat_sectors;
	u32 data_start_sector = root_start_sector + root_sectors;

	clusters = (total_sectors - data_start_sector) / sectors_per_cluster;

	if (clusters < 0x0ff5) fat_type = 12;
	else if (clusters < 0xfff5) fat_type = 16;
	else fat_type = 32;

	fat_offset = (u64)bytes_per_sector * fat_start_sector;
	root_offset = (u64)bytes_per_sector * root_start_sector;
	data_offset = (u64)bytes_per_sector * data_start_sector;

	if (fat_type == 320) root_offset = le32(sb + 0x2c);

	return 0;
}

static int is_fat_fs (const u8 *sb) {
	u32 bps = le16(sb + 0x0b);
	if (bps < 0x0200 || bps > 0x1000 || bps & (bps - 1)) return 0;

	if (sb[0x15] < 0xf8 && sb[0x15] != 0xf0) return 0;

	return 1;
}

int fat_init (void) {
	int part;
	u8 buf[0x200];
	int err;

	partition_start_offset = 0;
	err = read(buf, 0, 0x200);
	if (err) return err;

	if (le16(buf + 0x01fe) != 0xaa55) return -1;

	if (is_fat_fs(buf)) return fat_init_fs(buf);

	for (part = 0; part < 4; part++) {
		u8 *part_entry = buf + 0x1be + part * 16;
		if (part_entry[4] == 0) continue;

		partition_start_offset = 0x200ULL * le32(part_entry + 8);

		err = read(buf, 0, 0x200);
		if (err) return err;
		if (is_fat_fs(buf)) return fat_init_fs(buf);
	}

	return -1;
}

