#ifndef __UTIL_H
#define __UTIL_H
#include "types.h"

extern u8 __code_buffer[];

static inline u16 le16 (const u8 *p) {
	return p[0] | (p[1] << 8);
}

static inline u32 le32 (const u8 *p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static inline u32 read32 (u32 addr) {
	u32 x;
	asm volatile("lwz %0,0(%1) ; sync" : "=r"(x) : "b"(0xc0000000 | addr));
	return x;
}

static inline void write32 (u32 addr, u32 x) {
	asm("stw %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

static inline u16 read16 (u32 addr) {
	u16 x;
	asm volatile("lhz %0,0(%1) ; sync" : "=r"(x) : "b"(0xc0000000 | addr));
	return x;
}

static inline void write16 (u32 addr, u16 x) {
	asm("sth %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

static inline u32 virt_to_phys (const void *p) {
	return (u32)p & 0x7fffffff;
}

static inline void *phys_to_virt (u32 x) {
	return (void *)(x | 0x80000000);
}

#endif /* __UTIL_H */
