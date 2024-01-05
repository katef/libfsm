/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_HASH_H
#define FSM_HASH_H

#include <stdint.h>

#include "adt/common.h"
#include "fsm/fsm.h"

#if EXPENSIVE_CHECKS
#include <assert.h>
#endif

#define HASH_LOG_PROBES 0
/* #define HASH_PROBE_LIMIT 100 */

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static __inline__ uint64_t
hash_id(uint64_t id)
{
	/* xorshift* A1(12,25,27),
	 * from http://vigna.di.unimi.it/ftp/papers/xorshift.pdf */
	uint64_t x = id + 1;
	x ^= x >> 12; // a
	x ^= x << 25; // b
	x ^= x >> 27; // c
	return x * 2685821657736338717LLU;
}

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static __inline__ uint64_t
hash_ids(size_t count, const fsm_state_t *ids)
{
	uint64_t h = 0;
	for (size_t i = 0; i < count; i++) {
		h = hash_id(h ^ ids[i]);
#if EXPENSIVE_CHECKS
		if (i > 0) {
			assert(ids[i-1] <= ids[i]);
		}
#endif
	}
	return h;
}

#endif
