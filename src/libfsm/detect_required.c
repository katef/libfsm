/*
 * Copyright 2024 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <fsm/fsm.h>
#include <fsm/walk.h>
#include <fsm/pred.h>

#include <adt/edgeset.h>
#include <adt/u64bitset.h>

#include "internal.h"

#define LOG_BASE 0
#define LOG_PROGRESS (LOG_BASE + 0)
#define LOG_STEPS (LOG_BASE + 0)

/* More than one label */
#define LABEL_GROUP ((uint16_t)-1)

struct dr_env {
	const struct fsm *dfa;
	size_t steps;

	/* Number of times a unique label has been required -- this is a count so that going
	 * from 0 <-> 1 can set/clear the accumulator, but going from 1 -> 2 etc. does not. */
	size_t counts[256];
	struct bm current;
	bool first_end_state;
	struct bm overall;

	struct dr_stack {
		size_t used;
		size_t ceil;
		struct stack_frame {
			fsm_state_t state;
			uint16_t label; /* unique label followed to get here, or LABEL_GROUP */
			struct edge_group_iter iter;
		} *frames;
	} stack;
};

#define DEF_STACK_FRAMES 16

/* Check symbols[]: if there's more than one bit set, then set label to
 * LABEL_GROUP, otherwise set it to the single bit's character value.
 * At least one bit must be set. */
static void check_symbols(const struct edge_group_iter_info *info, uint16_t *label)
{
	bool any = false;

	for (size_t i = 0; i < 256/64; i++) {
		uint64_t w = info->symbols[i];
		if (w == 0) { continue; }

		/* get position of lowest set bit */
		for (size_t b = 0; b < 64; b++) {
			const uint64_t bit = 1ULL << b;
			if (w & bit) {
				if (any) {
					*label = LABEL_GROUP;
					return;
				}

				/* clear it, check if there's anything else set */
				w &= ~bit;
				if (w != 0) {
					*label = LABEL_GROUP;
					return;
				}

				*label = 64*i + b;
				any = true;
				break;
			}
		}
	}

	/* there must be at least one bit set */
	assert(any);
}

/* Walk a DFA and attempt to detect which characters must appear in any input to match.
 * This finds the intersection of characters required on any start->end paths (tracking
 * edges with only one label that must be followed by all matches), so it can take
 * prohibitively long for large/complex DFAs. */
enum fsm_detect_required_characters_res
fsm_detect_required_characters(const struct fsm *dfa, size_t step_limit,
	uint64_t charmap[4], size_t *count)
{
	assert(dfa != NULL);
	assert(charmap != NULL);

	#if EXPENSIVE_CHECKS
	if (!fsm_all(dfa, fsm_isdfa)) {
		return FSM_DETECT_REQUIRED_CHARACTERS_ERROR_MISUSE;
	}
	#endif

	enum fsm_detect_required_characters_res res = FSM_DETECT_REQUIRED_CHARACTERS_ERROR_ALLOC;

	struct dr_env env = {
		.dfa = dfa,
		.first_end_state = true,
	};

	assert(env.counts[0] == 0);

	const size_t state_count = fsm_countstates(dfa);
	fsm_state_t start_state;
	if (!fsm_getstart(dfa, &start_state)) {
		res = FSM_DETECT_REQUIRED_CHARACTERS_ERROR_MISUSE;
		goto cleanup;
	}

	#if EXPENSIVE_CHECKS
	for (fsm_state_t s = 0; s < state_count; s++) {
		assert(!dfa->states[s].visited);
	}
	#endif

	for (size_t i = 0; i < 4; i++) { charmap[i] = 0; }

	/* If the start state is also an end state, then
	 * it matches the empty string, so we're done. */
	if (fsm_isend(dfa, start_state)) {
		res = FSM_DETECT_REQUIRED_CHARACTERS_WRITTEN;
		if (count != NULL) {
			*count = 0;
		}
		goto cleanup;
	}

	env.stack.frames = f_malloc(dfa->alloc, DEF_STACK_FRAMES * sizeof(env.stack.frames[0]));
	if (env.stack.frames == NULL) { goto cleanup; }
	env.stack.ceil = DEF_STACK_FRAMES;

	{			/* set up start state's stack frame */
		struct stack_frame *sf0 = &env.stack.frames[0];
		sf0->state = start_state;
		sf0->label = LABEL_GROUP;

		dfa->states[start_state].visited = true;

		edge_set_group_iter_reset(dfa->states[start_state].edges,
		    EDGE_GROUP_ITER_ALL, &sf0->iter);
		env.stack.used = 1;
	}

	while (env.stack.used > 0) {
		struct stack_frame *sf = &env.stack.frames[env.stack.used - 1];
		struct edge_group_iter_info info;
		env.steps++;
		if (LOG_STEPS > 1) {
			fprintf(stderr, "-- steps %zu/%zu\n", env.steps, step_limit);
		}
		if (env.steps == step_limit) {
			res = FSM_DETECT_REQUIRED_CHARACTERS_STEP_LIMIT_REACHED;
			goto cleanup;
		}

		if (edge_set_group_iter_next(&sf->iter, &info)) {
			assert(info.to < state_count);
			if (dfa->states[info.to].visited) {
				continue; /* skip visited state */
			}

			if (env.stack.used == env.stack.ceil) { /* grow stack */
				const size_t nceil = 2*env.stack.ceil;
				assert(nceil > env.stack.ceil);
				struct stack_frame *nframes = f_realloc(dfa->alloc,
				    env.stack.frames, nceil * sizeof(nframes[0]));
				if (nframes == NULL) {
					return FSM_DETECT_REQUIRED_CHARACTERS_ERROR_ALLOC;
				}

				env.stack.frames = nframes;
				env.stack.ceil = nceil;
			}

			/* enter state */
			dfa->states[info.to].visited = true;

			struct stack_frame *nsf = &env.stack.frames[env.stack.used];
			nsf->state = info.to;
			check_symbols(&info, &nsf->label);

			if (nsf->label != LABEL_GROUP) {
				size_t offset = (nsf->label & 0xff);
				const size_t label_count = ++env.counts[offset];
				if (label_count == 1) {
					bm_set(&env.current, offset);
				}
			}

			edge_set_group_iter_reset(dfa->states[info.to].edges,
			    EDGE_GROUP_ITER_ALL, &nsf->iter);
			env.stack.used++;

			if (fsm_isend(dfa, info.to)) {
				if (env.first_end_state) {
					bm_copy(&env.overall, &env.current);
					env.first_end_state = false;
				} else { /* intersect */
					bm_intersect(&env.overall, &env.current);
				}

				if (LOG_PROGRESS) {
					fprintf(stderr, "-- current: ");
					bm_print(stderr, NULL, &env.current, 0, fsm_escputc);
					fprintf(stderr, ", overall: ");
					bm_print(stderr, NULL, &env.overall, 0, fsm_escputc);
					fprintf(stderr, "\n");
				}

				/* Intersecting with the empty set will always be empty, so
				 * further exploration is unnecessary. */
				if (!bm_any(&env.overall)) {
					res = FSM_DETECT_REQUIRED_CHARACTERS_WRITTEN;
					break;
				}
			}

		} else {	/* done with state */
			/* If this state was reached via a unique label, then
			 * reduce the count. If the count returns to 0, remove
			 * it from the constraint set. */
			if (sf->label != LABEL_GROUP) {
				size_t offset = (sf->label & 0xff);
				const size_t label_count = --env.counts[offset];
				if (label_count == 0) {
					bm_unset(&env.current, offset);
				}
			}

			/* clear visited */
			dfa->states[sf->state].visited = false;

			env.stack.used--;
		}
	}

	if (LOG_STEPS) {
		fprintf(stderr, "%s: finished in %zu/%zu steps\n", __func__, env.steps, step_limit);
	}

	for (size_t i = 0; i < 4; i++) {
		charmap[i] = *bm_nth_word(&env.overall, i);
	}

	if (count != NULL) {
		*count = bm_count(&env.overall);
	}

	res = FSM_DETECT_REQUIRED_CHARACTERS_WRITTEN;

cleanup:
	f_free(dfa->alloc, env.stack.frames);

	for (fsm_state_t s = 0; s < state_count; s++) {
		dfa->states[s].visited = false;
	}

	return res;
}
