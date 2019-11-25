/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/cost.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/set.h>
#include <adt/priq.h>
#include <adt/path.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

struct path *
fsm_shortest(const struct fsm *fsm,
	fsm_state_t start, fsm_state_t goal,
	unsigned (*cost)(fsm_state_t from, fsm_state_t to, char c))
{
	struct priq *todo, *done;
	struct priq *u;

	struct path *path;

	assert(fsm != NULL);
	assert(start < fsm->statecount);
	assert(goal < fsm->statecount);
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
	 *
	 * We limit ourselves to epsilon-free FSM only just for simplicity of the
	 * implementation; the algorithm itself works jsut as well for NFA if a
	 * cost can be assigned to epsilon transitions. Ensuring there are no
	 * epsilons makes for a simpler API.
	 */

	assert(!fsm_has(fsm, fsm_hasepsilons));

	todo = NULL;
	done = NULL;

	path = NULL;

	{
		fsm_state_t qs, i;

		if (!fsm_getstart(fsm, &qs)) {
			errno = EINVAL;
			goto error;
		}

		for (i = 0; i < fsm->statecount; i++) {
			u = priq_push(fsm->opt->alloc, &todo, i,
				i == qs ? 0 : FSM_COST_INFINITY);
			if (u == NULL) {
				goto error;
			}

			/*
			 * It's non-sensical to describe the start state as being reached
			 * by a symbol; this field is not used.
			 */
			if (i == qs) {
				u->c = '\0';
			}
		}
	}

	while (u = priq_pop(&todo), u != NULL) {
		struct edge_iter it;
		struct fsm_edge *e;
		fsm_state_t s;

		priq_move(&done, u);

		if (u->cost == FSM_COST_INFINITY) {
			goto none;
		}

		if (u->state == goal) {
			assert(u->prev != NULL || goal == start);
			goto done;
		}

		for (e = edge_set_first(fsm->states[u->state]->edges, &it); e != NULL; e = edge_set_next(&it)) {
			struct state_iter jt;

			for (state_set_reset(e->sl, &jt); state_set_next(&jt, &s); ) {
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
					v->c    = e->symbol;

					priq_update(&todo, v, v->cost);
				}
			}
		}
	}

done:

	for (u = priq_find(done, goal); u != NULL; u = u->prev) {
		if (!path_push(fsm->opt->alloc, &path, u->state, u->c)) {
			goto error;
		}
	}

	priq_free(fsm->opt->alloc, todo);
	priq_free(fsm->opt->alloc, done);

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

	if (!path_push(fsm->opt->alloc, &path, u->state, u->c)) {
		goto error;
	}

	priq_free(fsm->opt->alloc, todo);
	priq_free(fsm->opt->alloc, done);

	return path;

error:

	priq_free(fsm->opt->alloc, todo);
	priq_free(fsm->opt->alloc, done);

	path_free(fsm->opt->alloc, path);

	return NULL;
}

