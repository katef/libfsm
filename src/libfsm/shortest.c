/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/priq.h>
#include <adt/path.h>

#include <fsm/fsm.h>
#include <fsm/cost.h>

#include "internal.h"

struct path *
fsm_shortest(const struct fsm *fsm,
	const struct fsm_state *start, const struct fsm_state *goal,
	unsigned (*cost)(const struct fsm_state *from, const struct fsm_state *to, int c))
{
	struct priq *todo, *done;
	struct priq *u;

	struct path *path;

	assert(fsm != NULL);
	assert(start != NULL);
	assert(goal != NULL);
	assert(cost != NULL);

	/*
	 * We find a least-cost ("shortest") path by Dijkstra's algorithm.
	 *
	 * This implementation (like most of libfsm) favours simplicity over
	 * execution time, as the use-cases here aren't particularly intensive.
	 * Visited nodes are kept in a seperate "done" set than the unvisited
	 * "todo" nodes.
	 *
	 * Distance between edges (cost) is unsigned, with UINT_MAX to represent
	 * infinity.
	 */

	todo = NULL;
	done = NULL;

	path = NULL;

	{
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			u = priq_push(&todo, s, s == fsm_getstart(fsm) ? 0 : FSM_COST_INFINITY);
			if (u == NULL) {
				goto error;
			}

			/*
			 * We consider the first state to be reached by an epsilon;
			 * this is effectively the entry into the FSM.
			 */
			if (s == fsm_getstart(fsm)) {
				u->type = FSM_EDGE_EPSILON;
			}
		}
	}

	while (u = priq_pop(&todo), u != NULL) {
		const struct fsm_state *s;
		struct fsm_edge *e;
		struct set_iter it;

		priq_move(&done, u);

		if (u->cost == FSM_COST_INFINITY) {
			goto none;
		}

		if (u->state == goal) {
			assert(u->prev != NULL || goal == fsm->start);
			goto done;
		}

		for (e = set_first(u->state->edges, &it); e != NULL; e = set_next(&it)) {
			struct set_iter jt;

			for (s = set_first(e->sl, &jt); s != NULL; s = set_next(&jt)) {
				struct priq *v;
				unsigned c;

				v = priq_find(todo, s);

				/* visited already */
				if (v == NULL) {
					assert(priq_find(done, s));
					continue;
				}

				assert(v->state == s);

				c = cost(u->state, v->state, e->symbol);

				/* relax */
				if (v->cost > u->cost + c) {
					v->cost = u->cost + c;
					v->prev = u;
					v->type = e->symbol;
				}
			}
		}
	}

done:

	for (u = priq_find(done, goal); u != NULL; u = u->prev) {
		if (!path_push(&path, u->state, u->type)) {
			goto error;
		}
	}

	priq_free(todo);
	priq_free(done);

	return path;

none:

	/*
	 * No known path to goal.
	 *
	 * This is slightly ugly: the idea is to return something non-NULL here;
	 * so that the caller may distinguish this case from error.
	 *
	 * The state u is used just because it may never be the goal state,
	 * (otherwise we would have been able to reach it).
	 */

	if (!path_push(&path, u->state, u->type)) {
		goto error;
	}

	priq_free(todo);
	priq_free(done);

	return path;

error:

	priq_free(todo);
	priq_free(done);

	path_free(path);

	return NULL;
}

