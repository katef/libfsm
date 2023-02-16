/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_HASH_H
#define FSM_HASH_H

#include <stdint.h>

#include "common/check.h"

/* 32 and 64-bit approximations of the golden ratio. */
#define FSM_PHI_32 0x9e3779b9UL
#define FSM_PHI_64 (uint64_t)0x9e3779b97f4a7c15UL

/* A suitable hash function for individual sequentially allocated
 * identifiers. See Knuth 6.4, Fibonacci hashing. */

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static __inline__ uint64_t
fsm_hash_id(unsigned id)
{
	return FSM_PHI_64 * (uint64_t)(id + (unsigned)1);
}

/* FNV-1a hash function, 32 and 64 bit versions. This is in the public
 * domain. For details, see:
 *
 *     http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static __inline__ uint32_t
fsm_hash_fnv1a_32(const uint8_t *buf, size_t length)
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
fsm_hash_fnv1a_64(const uint8_t *buf, size_t length)
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
