// Copyright 2009 Alan Williams <mralert@gmail.com>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "string.h"

int memcmp (const void *s1, const void *s2, size_t n) {
	const char *a = s1;
	const char *b = s2;
	for (; n > 0; n--, a++, b++) {
		if (*a < *b) return -1;
		if (*a > *b) return 1;
	}
	return 0;
}

void *memcpy (void *dest, const void *src, size_t n) {
	const char *s = src;
	char *d = dest;
	for (; n > 0; n--) *d++ = *s++;
	return dest;
}

void *memset (void *s, int c, size_t n) {
	char *r = s;
	for (; n > 0; n--) *r++ = c;
	return s;
}

size_t strlen (const char *s) {
	size_t i;
	for (i = 0; *s; i++, s++);
	return i;
}
