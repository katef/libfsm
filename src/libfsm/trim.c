/*
 * Copyright 2019 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <adt/queue.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>

#include "internal.h"

static void
clear_states(struct fsm_state *s)
{
	for ( ; s != NULL; s = s->next) {
		s->reachable = 0;
	}
}

static int
mark_states(struct fsm *fsm)
{
	const unsigned state_count = fsm_countstates(fsm);
	struct queue *q;
	int res = 0;

	q = queue_new(fsm->opt->alloc, state_count);
	if (q == NULL) {
		return 1;
	}

	if (!queue_push(q, fsm->start)) {
		goto cleanup;
	}
	fsm->start->reachable = 1;

	for (;;) {
		const struct fsm_edge *e;
		struct edge_iter edge_iter;
		struct fsm_state *s;

		/* pop off queue; break if empty */
		if (!queue_pop(q, (void *)&s)) { break; }

		/* enqueue all directly reachable and unmarked, and mark them */
		{
			struct state_iter state_iter;
			struct fsm_state *es;

			for (es = state_set_first(s->epsilons, &state_iter);
			     es != NULL;
			     es = state_set_next(&state_iter))
			{
				if (es->reachable) {
					continue;
				}

				if (!queue_push(q, es)) {
					goto cleanup;
				}

				es->reachable = 1;
			}
		}

		for (e = edge_set_first(s->edges, &edge_iter);
		     e != NULL;
		     e = edge_set_next(&edge_iter))
		{
			struct state_iter state_iter;
			struct fsm_state *es;

			for (es = state_set_first(e->sl, &state_iter);
			     es != NULL;
			     es = state_set_next(&state_iter))
			{
				if (es->reachable) {
					continue;
				}

				if (!queue_push(q, es)) {
					goto cleanup;
				}

				es->reachable = 1;
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

int
sweep_states(struct fsm *fsm)
{
	struct fsm_state *prev, *s, *next;
	struct fsm_state **new_tail;
	int swept;

	prev = NULL;
	s = fsm->sl;
	new_tail = &fsm->sl;
	swept = 0;

	/*
	 * This doesn't use fsm_removestate because it would be modifying the
	 * state graph while traversing it, and any state being removed here
	 * should (by definition) not be the start, or have any other reachable
	 * edges referring to it.
	 *
	 * There may temporarily be other states in the graph with other edges
	 * to it, because the states aren't topologically sorted, but
	 * they'll be collected soon as well.
	 */
	while (s != NULL) {
		next = s->next;

		if (!s->reachable) {
			assert(s != fsm->start);

			/* for endcount accounting */
			fsm_setend(fsm, s, 0);

			/* unlink */
			if (prev != NULL) {
				prev->next = next;
			}
			if (fsm->sl == s) {
				fsm->sl = next;
			}

			edge_set_free(s->edges);
			free(s);
			swept++;
		} else {
			new_tail = &s->next;
			prev = s;
		}

		s = next;
	}

	fsm->tail = new_tail;
	return swept;
}

int
fsm_trim(struct fsm *fsm)
{
	clear_states(fsm->sl);

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

	return sweep_states(fsm);
}

