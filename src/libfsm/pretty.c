/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/out.h>
#include <fsm/options.h>

#include "internal.h"

static int
selfepsilonstate(struct fsm_state *s, int *changed)
{
	struct set_iter it;
	struct fsm_state *to;
	struct fsm_edge *e;

next1:

	for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
		struct set_iter jt;

		if (e->symbol != FSM_EDGE_EPSILON) {
			continue;
		}

next0:

		for (to = set_first(e->sl, &jt); to != NULL; to = set_next(&jt)) {
			if (to != s) {
				continue;
			}

			set_remove(&e->sl, to);

			*changed = 1;

			goto next0; /* XXX */
		}

		if (set_empty(e->sl)) {
			set_remove(&s->edges, e);

			goto next1; /* XXX */
		}
	}

	return 1;
}

/* TODO: centralise */
static struct fsm_state *
merge(struct fsm *fsm, struct fsm_state *x, struct fsm_state *y)
{
	struct fsm_state *q;
	struct set *dummy;
	int start, end;

	/* XXX: seems like something fsm_mergestates() ought to do */
	end   = fsm_isend(fsm, x) || fsm_isend(fsm, y);
	start = fsm_getstart(fsm) == x || fsm_getstart(fsm) == y;

	if (end) {
		dummy = NULL;

		if (fsm_isend(fsm, x)) {
			void *opaque;

			opaque = fsm_getopaque(fsm, x);
			if (opaque != NULL && !set_add(&dummy, opaque)) {
				goto error;
			}
		}

		if (fsm_isend(fsm, y)) {
			void *opaque;

			opaque = fsm_getopaque(fsm, y);
			if (opaque != NULL && !set_add(&dummy, opaque)) {
				goto error;
			}
		}
	}

	q = fsm_mergestates(fsm, x, y);
	if (q == NULL) {
		return NULL;
	}

	if (start) {
		fsm_setstart(fsm, q);
	}

	if (end) {
		fsm_setend(fsm, q, 1);

		if (!set_empty(dummy)) {
			fsm_carryopaque(fsm, dummy, fsm, q);

			set_free(dummy);
		}
	}

	{
		int dummy;

		if (!selfepsilonstate(q, &dummy)) {
			return 0;
		}

		(void) dummy;
	}

	return q;

error:

	return NULL;
}

/* XXX: super expensive */
static struct fsm_state *
singleepsilonsource(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *to, *from;
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(state != NULL);

	from = NULL;

	for (s = fsm->sl; s != NULL; s = s->next) {
		struct fsm_edge *e;
		struct set_iter it, jt;

		if (s == state) {
			continue;
		}

		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			for (to = set_first(e->sl, &jt); to != NULL; to = set_next(&jt)) {
				if (to != state) {
					continue;
				}

				if (from != NULL) {
					return NULL;
				}

				if (e->symbol != FSM_EDGE_EPSILON) {
					goto next0; /* XXX */
				}

				from = s;
			}
		}

next0: ;

	}

	return from;
}

static struct fsm_state *
singleepsilontarget(const struct fsm_state *state)
{
	struct fsm_edge *e;
	struct fsm_state *to;

	assert(state != NULL);

	if (set_count(state->edges) != 1) {
		return NULL;
	}

	e = set_only(state->edges);
	assert(e != NULL);

	if (e->symbol != FSM_EDGE_EPSILON) {
		return NULL;
	}

	if (set_count(e->sl) != 1) {
		return NULL;
	}

	to = set_only(e->sl);
	if (to == NULL) {
		return NULL;
	}

	return to;
}

/*
 * Remove all epsilon edges looping to the same state.
 */
static int
selfepsilons(struct fsm *fsm, int *changed)
{
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(changed != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (!selfepsilonstate(s, changed)) {
			return 0;
		}
	}

	return 1;
}

/*
 * If the only outgoing edge is a single epsilon, combine states.
 */
static int
dstepsilons(struct fsm *fsm, int *changed)
{
	struct fsm_state *s, *next;

	assert(fsm != NULL);
	assert(changed != NULL);

	for (s = fsm->sl; s != NULL; s = next) {
		struct fsm_state *to;
		struct fsm_state *q;

		next = s->next;

		to = singleepsilontarget(s);
		if (to == NULL) {
			continue;
		}

		if (to == s) {
			continue;
		}

		q = merge(fsm, s, to);
		if (q == NULL) {
			return 0;
		}

		next = q;

		*changed = 1;
	}

	return 1;
}

/*
 * If a state's only incoming edge is an epsilon,
 * merge it with that epsilon's source.
 */
static int
srcepsilons(struct fsm *fsm, int *changed)
{
	struct fsm_state *s, *next;
	struct fsm_state *from;

	assert(fsm != NULL);
	assert(changed != NULL);

	for (s = fsm->sl; s != NULL; s = next) {
		struct fsm_state *q;

		next = s->next;

		from = singleepsilonsource(fsm, s);
		if (from == NULL) {
			continue;
		}

		if (from == s) {
			continue;
		}

		q = merge(fsm, from, s);
		if (q == NULL) {
			return 0;
		}

		next = q;

		*changed = 1;
	}

	return 1;
}

static int
endepsilons(struct fsm *fsm, int *changed)
{
	struct fsm_state *s;

	for (s = fsm->sl; s != NULL; s = s->next) {
		struct fsm_state *to;
		struct fsm_edge *e;
		struct set_iter it, jt;

		if (!fsm_isend(fsm, s)) {
			continue;
		}

		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			if (e->symbol != FSM_EDGE_EPSILON) {
				continue;
			}

next0:

			for (to = set_first(e->sl, &jt); to != NULL; to = set_next(&jt)) {
				if (!fsm_isend(fsm, to)) {
					continue;
				}

				/* if the end state has outgoing edges,
				 * skip removing the epsilon under consideration */
				if (fsm_hasoutgoing(fsm, to)) {
					goto next1; /* XXX */
				}

				set_remove(&e->sl, to);

				*changed = 1;

				goto next0; /* XXX */
			}
		}

next1: ;

	}

	return 1;
}

static int
propogateend(struct fsm *fsm, int *changed)
{
	struct fsm_state *s;

	for (s = fsm->sl; s != NULL; s = s->next) {
		struct fsm_state *to;
		struct fsm_edge *e;
		struct set_iter it, jt;

		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			if (e->symbol != FSM_EDGE_EPSILON) {
				continue;
			}

			for (to = set_first(e->sl, &jt); to != NULL; to = set_next(&jt)) {
				if (!fsm_isend(fsm, to)) {
					continue;
				}

				if (fsm_hasoutgoing(fsm, to)) {
					continue;
				}

				if (!fsm_isend(fsm, s)) {
					struct set *dummy;
					void *opaque;

					fsm_setend(fsm, s, 1);

					dummy = NULL;

					opaque = fsm_getopaque(fsm, to);
					if (opaque != NULL && !set_add(&dummy, opaque)) {
						return 0; /* XXX */
					}

					if (!set_empty(dummy)) {
						fsm_carryopaque(fsm, dummy, fsm, s);
					}

					set_free(dummy);

					*changed = 1;
				}
			}
		}
	}

	return 1;
}

int
fsm_pretty(struct fsm *fsm)
{
	int changed;

	assert(fsm != NULL);

/* XXX: doesn't work here... why not?
	do {
		changed = 0;

		if (!propogateend(fsm, &changed)) {
			return 0;
		}

	} while (changed);
*/

	do {
		changed = 0;

		if (!propogateend(fsm, &changed)) {
			return 0;
		}

		if (!endepsilons(fsm, &changed)) {
			return 0;
		}

		if (!selfepsilons(fsm, &changed)) {
			return 0;
		}

		if (!dstepsilons(fsm, &changed)) {
			return 0;
		}

		if (!srcepsilons(fsm, &changed)) {
			return 0;
		}

		if (!fsm_trim(fsm)) {
			return 0;
		}

	} while (changed);

	/* TODO: remove redundant alts */
	/* TODO: collapse prefixes and suffixes:
	 * brzowoski's algorithm but with the powerset construction treating epsilon
	 * as just another symbol. I think this is identical to hopcroft's equivalence function;
	 * probably add that to libfsm just by itself. */

	/* TODO: afterwards: split out end states with only incoming epsilon edges;
	 * this helps show tries */

	return 1;
}

