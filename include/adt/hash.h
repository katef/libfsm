/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_HASH_H
#define FSM_HASH_H

#include <stdint.h>

#include "adt/common.h"

/* 32 and 64-bit approximations of the golden ratio. */
#define FSM_PHI_32 0x9e3779b9UL
#define FSM_PHI_64 (uint64_t)0x9e3779b97f4a7c15UL

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static __inline__ uint64_t
hash_id(unsigned id)
{
	/* xorshift* A1(12,25,27),
	 * from http://vigna.di.unimi.it/ftp/papers/xorshift.pdf */
	uint64_t x = id + 1;
	x ^= x >> 12; // a
	x ^= x << 25; // b
	x ^= x >> 27; // c
	return x * 2685821657736338717LLU;
}

/* FNV-1a hash function, 32 and 64 bit versions. This is in the public
 * domain. For details, see:
 *
 *     http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static __inline__ uint32_t
hash_fnv1a_32(const uint8_t *buf, size_t length)
{
#define FNV1a_32_OFFSET_BASIS 	0x811c9dc5UL
#define FNV1a_32_PRIME		0x01000193UL
	uint32_t h = FNV1a_32_OFFSET_BASIS;
	size_t i;
	for (i = 0; i < length; i++) {
		h ^= buf[i];
		h *= FNV1a_32_PRIME;
	}
	return h;
}

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static __inline__ uint64_t
hash_fnv1a_64(const uint8_t *buf, size_t length)
{
#define FNV1a_64_OFFSET_BASIS   0xcbf29ce484222325UL
#define FNV1a_64_PRIME		0x100000001b3UL
	uint64_t h = FNV1a_64_OFFSET_BASIS;
	size_t i;
	for (i = 0; i < length; i++) {
		h ^= buf[i];
		h *= FNV1a_64_PRIME;
	}
	return h;
}

#endif
