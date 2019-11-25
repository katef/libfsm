/*
 * Copyright 2019 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <fsm/fsm.h>

#include <adt/queue.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>

#include "internal.h"

static int
mark_states(struct fsm *fsm)
{
	const unsigned state_count = fsm_countstates(fsm);
	fsm_state_t start;
	struct queue *q;
	int res = 0;

	q = queue_new(fsm->opt->alloc, state_count);
	if (q == NULL) {
		return 1;
	}

	if (!fsm_getstart(fsm, &start)) {
		return 1;
	}

	if (!queue_push(q, start)) {
		goto cleanup;
	}
	fsm->states[start]->reachable = 1;

	for (;;) {
		const struct fsm_edge *e;
		struct edge_iter edge_iter;
		fsm_state_t s;

		/* pop off queue; break if empty */
		if (!queue_pop(q, &s)) { break; }

		/* enqueue all directly reachable and unmarked, and mark them */
		{
			struct state_iter state_iter;
			fsm_state_t es;

			for (state_set_reset(fsm->states[s]->epsilons, &state_iter); state_set_next(&state_iter, &es); ) {
				if (fsm->states[es]->reachable) {
					continue;
				}

				if (!queue_push(q, es)) {
					goto cleanup;
				}

				fsm->states[es]->reachable = 1;
			}
		}

		for (e = edge_set_first(fsm->states[s]->edges, &edge_iter);
		     e != NULL;
		     e = edge_set_next(&edge_iter))
		{
			struct state_iter state_iter;
			fsm_state_t es;

			for (state_set_reset(e->sl, &state_iter); state_set_next(&state_iter, &es); ) {
				if (fsm->states[es]->reachable) {
					continue;
				}

				if (!queue_push(q, es)) {
					goto cleanup;
				}

				fsm->states[es]->reachable = 1;
			}
		}
	}

	res = 1;

cleanup:

	if (q != NULL) {
		free(q);
	}

	return res;
}

long
sweep_states(struct fsm *fsm)
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
		if (fsm->states[i]->reachable) {
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
	fsm_state_t i;

	assert(fsm != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		fsm->states[i]->reachable = 0;
	}

	if (!mark_states(fsm)) {
		return -1;
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
	ret = sweep_states(fsm);
	if (ret < 0) {
		return ret;
	}
	
	return 1;
}

