/*
 * Copyright 2023 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdlib.h>
#include <stdint.h>
#include <fsm/fsm.h>
#include <fsm/alloc.h>

#include <assert.h>

#include "internal.h"

static void
populate_mapping(fsm_state_t *mapping, size_t count, uint32_t seed)
{
	/* Use a linear congruential pseudorandom number generator
	 * to populate mapping with a shuffled sequence of unique
	 * values 0..count. */
	uint32_t a = 4LU * seed;
	a = (a ? a : 4) | 1;

	const uint32_t c = 0x7fffffff; /* large prime */
	/* mask out upper bits to avoid overflow when multiplying */
	uint32_t state = seed & 0x1fffffff;

	unsigned mask = 1;
	while (mask <= count) {
		mask <<= 1;
	}
	mask--;
	assert(mask >= count);

	size_t i = 0;
	while (i < count) {
		state = ((a * state) + c) & mask;
		if (state < count) {
			mapping[i] = state;
			i++;
		} else {
			/* out of range, redraw */
		}
	}

#if EXPENSIVE_CHECKS
	/* no duplicates */
	for (size_t i = 0; i < count; i++) {
		fsm_state_t cur = mapping[i];
		for (size_t j = i + 1; j < count; j++) {
			assert(cur != mapping[j]);
		}
	}
#endif

}

int
fsm_shuffle(struct fsm *fsm, unsigned seed)
{
	int res = 0;

	struct fsm *dst = NULL;
	fsm_state_t *mapping = NULL;
	const size_t state_count = fsm_countstates(fsm);
	if (state_count <= 1) {
		return 1;
	}

	mapping = f_malloc(fsm->opt->alloc, state_count * sizeof(mapping[0]));
	if (mapping == NULL) { goto cleanup; }

	populate_mapping(mapping, state_count, (uint32_t)seed);

	dst = fsm_consolidate(fsm, mapping, state_count);
	fsm_move(fsm, dst);
	dst = NULL;

	res = 1;

cleanup:
	f_free(fsm->opt->alloc, mapping);
	return res;
}
