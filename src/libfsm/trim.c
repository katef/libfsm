/*
 * Copyright 2019 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>

#include "internal.h"

enum mark_flags {
	MARK_UNVISITED = 0x00,
	MARK_ENQUEUED = 0x01,
	MARK_PROCESSING = 0x02,
	MARK_REACHES_END = 0x04,
	MARK_DONE = 0x08
};

static int
mark_states(struct fsm *fsm,
	unsigned char *marks)
{
	/* Mark all states that are reachable from the start state and
	 * that are either ends or have edge(s) which lead to an edge
	 * state (directly or transitively).
	 *
	 * This uses a heap-allocated stack, because a recursive call
	 * could overrun the call stack for sufficiently large and
	 * deeply nested automata, and each recursive call only needs
	 * the state ID and the flags in marks[]. This slightly
	 * complicates what would otherwise be a pretty simple recursive
	 * algorithm with memoization. */
	const unsigned state_count = fsm_countstates(fsm);

	/* Basic stack: Empty when i == 0, top at i - 1. */
	fsm_state_t *stack = NULL;
	size_t stack_i = 0;
	size_t stack_ceil;

	fsm_state_t start;
	if (!fsm_getstart(fsm, &start)) {
		return 1;
	}

	stack_ceil = state_count + 1;
	if (stack_ceil == 0) {
		stack_ceil = 1;
	}
	stack = f_malloc(fsm->opt->alloc,
	    stack_ceil * sizeof(stack[0]));
	if (stack == NULL) {
		return 0;
	}

	stack[stack_i] = start;
	stack_i++;
	marks[start] |= MARK_ENQUEUED;

	while (stack_i > 0) {
		fsm_state_t s = stack[stack_i - 1];
		struct fsm_edge e;
		struct edge_iter edge_iter;
		struct state_iter state_iter;
		fsm_state_t epsilon_s;

		if (!(marks[s] & MARK_PROCESSING)) {
			/* First pass: Note that processing has started,
			 * stack other reachable states, note if this is
			 * an end state. */
			marks[s] |= MARK_PROCESSING;

			if (fsm_isend(fsm, s)) {
				marks[s] |= MARK_REACHES_END;
			}

			for (state_set_reset(fsm->states[s].epsilons, &state_iter); state_set_next(&state_iter, &epsilon_s); ) {
				if (marks[epsilon_s] & MARK_PROCESSING) {
					continue; /* already being processed */
				}

				if (stack_i == stack_ceil) {
					size_t nceil = 2*stack_ceil;
					fsm_state_t *nstack = f_realloc(fsm->opt->alloc,
					    stack, nceil * sizeof(stack[0]));
					assert(!"unreachable");
					if (stack == NULL) {
						f_free(fsm->opt->alloc, stack);
						return 0;
					}
					stack_ceil = nceil;
					stack = nstack;
				}

				if (!(marks[epsilon_s] & MARK_ENQUEUED)) {
					stack[stack_i] = epsilon_s;
					stack_i++;
					marks[epsilon_s] |= MARK_ENQUEUED;
				}
			}

			for (edge_set_reset(fsm->states[s].edges, &edge_iter); edge_set_next(&edge_iter, &e); ) {
				if (marks[e.state] & MARK_PROCESSING) {
					continue;
				}

				if (stack_i == stack_ceil) {
					size_t nceil = 2*stack_ceil;
					fsm_state_t *nstack = f_realloc(fsm->opt->alloc,
					    stack, nceil * sizeof(stack[0]));
					assert(!"unreachable");
					if (stack == NULL) {
						f_free(fsm->opt->alloc, stack);
						return 0;
					}
					stack_ceil = nceil;
					stack = nstack;
				}

				if (!(marks[e.state] & MARK_ENQUEUED)) {
					stack[stack_i] = e.state;
					stack_i++;
					marks[e.state] |= MARK_ENQUEUED;
				}
			}
		} else if (!(marks[s] & MARK_DONE)) {
			/* Second pass: Mark the state as reaching an
			 * end if any of its reachable nodes do, mark
			 * processing as done. Terminate iteration once
			 * the END flag is set. */
			for (state_set_reset(fsm->states[s].epsilons, &state_iter);
			     state_set_next(&state_iter, &epsilon_s); ) {
				if (marks[s] & MARK_REACHES_END) {
					break;
				}
				if (marks[epsilon_s] & MARK_REACHES_END) {
					marks[s] |= MARK_REACHES_END;
				}
			}

			for (edge_set_reset(fsm->states[s].edges, &edge_iter);
			     edge_set_next(&edge_iter, &e); ) {
				if (marks[s] & MARK_REACHES_END) {
					break;
				}
				if (marks[e.state] & MARK_REACHES_END) {
					marks[s] |= MARK_REACHES_END;
				}
			}

			marks[s] |= MARK_DONE;
			stack_i--;
		} else {
			assert(!"unreachable");
		}
	}

	f_free(fsm->opt->alloc, stack);

	return 1;
}

static int
marks_filter_cb(fsm_state_t id, void *opaque)
{
	const unsigned char *marks = opaque;
	return marks[id] & MARK_REACHES_END;
}

long
sweep_states(struct fsm *fsm,
	unsigned char *marks)
{
	size_t swept;

	assert(fsm != NULL);

	if (!fsm_compact_states(fsm, marks_filter_cb, marks, &swept)) {
		return -1;
	}

	return (long)swept;
}

int
fsm_trim(struct fsm *fsm)
{
	long ret;
	unsigned char *marks = NULL;
	assert(fsm != NULL);

	/* Strictly speaking, this only needs 4 bits per state,
	 * but using a full byte makes the addressing easier. */
	marks = f_calloc(fsm->opt->alloc,
	    fsm->statecount, sizeof(marks[0]));
	if (marks == NULL) {
		goto cleanup;
	}

	if (!mark_states(fsm, marks)) {
		goto cleanup;
	}

	/*
	 * Remove all states which are unreachable from the start state
	 * or have no path to an end state. These are a trailing suffix
	 * which will never accept.
	 *
	 * sweep_states returns a negative value on error, otherwise it returns
	 * the number of states swept.
	 */
	ret = sweep_states(fsm, marks);

	f_free(fsm->opt->alloc, marks);

	if (ret < 0) {
		return ret;
	}

	return 1;

cleanup:
	if (marks != NULL) {
		f_free(fsm->opt->alloc, marks);
	}
	return -1;
}
