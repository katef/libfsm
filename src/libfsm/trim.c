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

				if (!marks[epsilon_s] & MARK_ENQUEUED) {
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

				if (!marks[e.state] & MARK_ENQUEUED) {
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

long
sweep_states(struct fsm *fsm,
	unsigned char *marks)
{
	long swept;
	fsm_state_t i;

	swept = 0;

	assert(fsm != NULL);

	/*
	 * This could be made faster by not using fsm_removestate (which does the
	 * work of also removing transitions from other states to the state being
	 * removed). We could get away with removing just our candidate state
	 * without removing transitions to it, because any state being removed here
	 * should (by definition) not be the start, or have any other reachable
	 * edges referring to it.
	 *
	 * There may temporarily be other states in the graph with other edges
	 * to it, because the states aren't topologically sorted, but
	 * they'll be collected soon as well.
	 *
	 * XXX: For now we call fsm_removestate() for simplicity of implementation.
	 */
	i = 0;
	while (i < fsm->statecount) {
		if (marks[i] & MARK_REACHES_END) {
			i++;
			continue;
		}

		/*
		 * This state cannot contain transitions pointing to earlier
		 * states, because they have already been removed. So we know
		 * the current index may not decrease.
		 */
		fsm_removestate(fsm, i);
		swept++;
	}

	return swept;
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
	 * Remove all states which have no reachable end state henceforth.
	 * These are a trailing suffix which will never accept.
	 *
	 * It doesn't matter which order in which these are removed;
	 * removing a state in the middle will disconnect the remainer of
	 * the suffix. The nodes in that newly disjoint subgraph
	 * will still be found to have no reachable end state, and so are
	 * also removed.
	 */

	/*
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
