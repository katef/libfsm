/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>
#include <adt/u64bitset.h>

#include "internal.h"

fsm_state_t
fsm_findmode(const struct fsm *fsm, fsm_state_t state, unsigned int *freq)
{
	struct edge_group_iter iter;
	struct edge_group_iter_info info;

	struct {
		fsm_state_t state;
		unsigned int freq;
	} mode;

	mode.freq = 1;
	mode.state = (fsm_state_t)-1;

	edge_set_group_iter_reset(fsm->states[state].edges, EDGE_GROUP_ITER_ALL, &iter);
	while (edge_set_group_iter_next(&iter, &info)) {
		unsigned curr = 0;
		for (size_t i = 0; i < 4; i++) {
			const uint8_t count = u64bitset_popcount(info.symbols[i]);
			curr += count;
		}
		if (curr > mode.freq) {
			mode.freq  = curr;
			mode.state = info.to;
		}
	}

	if (freq != NULL) {
		*freq = mode.freq;
	}

	/* It's not meaningful to call this on a state without edges. */
	assert(mode.state != (fsm_state_t)-1);

	assert(mode.freq >= 1);
	return mode.state;
}
