/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/priq.h>
#include <adt/path.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/cost.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

struct path *
fsm_shortest(const struct fsm *fsm,
	const struct fsm_state *start, const struct fsm_state *goal,
	unsigned (*cost)(const struct fsm_state *from, const struct fsm_state *to, char c))
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
		size_t i;

		for (i = 0; i < fsm->statecount; i++) {
			u = priq_push(fsm->opt->alloc, &todo, fsm->states[i],
				fsm->states[i] == fsm_getstart(fsm) ? 0 : FSM_COST_INFINITY);
			if (u == NULL) {
				goto error;
			}

			/*
			 * It's non-sensical to describe the start state as being reached
			 * by a symbol; this field is not used.
			 */
			if (fsm->states[i] == fsm_getstart(fsm)) {
				u->c = '\0';
			}
		}
	}

	while (u = priq_pop(&todo), u != NULL) {
		const struct fsm_state *s;
		struct fsm_edge *e;
		struct edge_iter it;

		priq_move(&done, u);

		if (u->cost == FSM_COST_INFINITY) {
			goto none;
		}

		if (u->state == goal) {
			assert(u->prev != NULL || goal == fsm->start);
			goto done;
		}

		for (e = edge_set_first(u->state->edges, &it); e != NULL; e = edge_set_next(&it)) {
			struct state_iter jt;

			for (s = state_set_first(e->sl, &jt); s != NULL; s = state_set_next(&jt)) {
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

