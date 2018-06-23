/*
 * Copyright 2008-2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/out.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include "libfsm/internal.h"
#include "libfsm/out.h"

#include "ir.h"

static unsigned int
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	unsigned int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 0; s != NULL; s = s->next, i++) {
		if (s == state) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

static const struct ir_state *
equivalent_cs(const struct fsm *fsm, const struct fsm_state *state,
	const struct ir *ir)
{
	assert(fsm != NULL);

	if (state == NULL) {
		return NULL;
	}

	return &ir->states[indexof(fsm, state)];
}

static struct ir_range *
make_ranges(const struct fsm *fsm, const struct fsm_state *state, const struct fsm_state *mode,
	const struct ir *ir, size_t *n)
{
	struct fsm_edge *e;
	struct set_iter it;
	struct ir_range *edges;

	assert(fsm != NULL);
	assert(state != NULL);
	assert(n != NULL);

	edges = malloc(sizeof *edges * UCHAR_MAX); /* worst case */
	if (edges == NULL) {
		/* XXX: bail out */
		return NULL;
	}

	*n = 0;

	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		struct fsm_state *s;

		if (e->symbol > UCHAR_MAX) {
			break;
		}

		if (set_empty(e->sl)) {
			continue;
		}

		s = set_only(e->sl);
		if (s == mode) {
			continue;
		}

		edges[*n].start = e->symbol;
		edges[*n].end   = e->symbol;

		edges[*n].to = equivalent_cs(fsm, s, ir);

		if (e->symbol <= UCHAR_MAX - 1) {
			do {
				const struct fsm_edge *ne;
				const struct fsm_state *ns;
				struct set_iter jt;

				ne = set_firstafter(state->edges, &jt, e);
				if (ne == NULL || ne->symbol != e->symbol + 1) {
					break;
				}

				if (set_empty(ne->sl)) {
					break;
				}

				ns = set_only(ne->sl);
				if (ns == mode || ns != s) {
					break;
				}

				edges[*n].end = ne->symbol;

				e = set_next(&it);
			} while (e != NULL);
		}

		(*n)++;
	}

	assert(*n > 0);

	{
		void *tmp;

		tmp = realloc(edges, sizeof *edges * *n);
		if (tmp == NULL) {
			/* XXX: bail out */
		}

		edges = tmp;
	}

	return edges;
}

static int
make_state(const struct fsm *fsm,
	struct fsm_state *state,
	const struct ir *ir, struct ir_state *cs)
{
	struct {
		struct fsm_state *state;
		unsigned int freq;
	} mode;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(state != NULL);
	assert(ir != NULL);

	/* TODO: IR_TABLE */

	/* no edges */
	{
		struct fsm_edge *e;
		struct set_iter it;

		e = set_first(state->edges, &it);
		if (!e || e->symbol > UCHAR_MAX) {
			cs->strategy = IR_NONE;
			return 0;
		}
	}

	if (fsm_iscomplete(fsm, state)) {
		mode.state = fsm_findmode(state, &mode.freq);
	} else {
		mode.state = NULL;
	}

	/* all edges go to the same state */
	if (mode.state != NULL && mode.freq == UCHAR_MAX) {
		cs->strategy  = IR_SAME;
		cs->u.same.to = equivalent_cs(fsm, mode.state, ir);
		return 0;
	}

	/* one dominant mode */
	if (mode.state != NULL) {
		cs->strategy = IR_MODE;

		cs->u.mode.ranges = make_ranges(fsm, state, mode.state, ir, &cs->u.mode.n);
		if (cs->u.mode.ranges == NULL) {
			/* TODO: bail out */
		}

		cs->u.mode.mode = equivalent_cs(fsm, mode.state, ir);
		return 0;
	}

	/* usual case */
	{
		cs->strategy = IR_MANY;

		cs->u.many.ranges = make_ranges(fsm, state, NULL, ir, &cs->u.many.n);
		if (cs->u.many.ranges == NULL) {
			/* TODO: bail out */
		}
	}

	return 0;
}

struct ir *
make_ir(const struct fsm *fsm)
{
	struct ir *ir;
	struct fsm_state *s;
	size_t i;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(fsm_all(fsm, fsm_isdfa));

	/* TODO: prerequisite that the FSM is a DFA */
	assert(fsm->start != NULL);

	ir = malloc(sizeof *ir);
	if (ir == NULL) {
		return NULL;
	}

	ir->n      = fsm_countstates(fsm);
	ir->states = malloc(sizeof *ir->states * ir->n);
	if (ir->states == NULL) {
		free(ir);
		return NULL;
	}

	for (s = fsm->sl, i = 0; s != NULL; s = s->next, i++) {
		assert(i < ir->n);

		ir->states[i].isend  = fsm_isend(fsm, s);
		ir->states[i].opaque = s->opaque;

		if (s == fsm->start) {
			ir->start = &ir->states[i];
		}

		if (make_state(fsm, s, ir, &ir->states[i]) == -1) {
			goto error;
		}

		if (!fsm->opt->comments) {
			ir->states[i].example = NULL;
		} else {
			/* XXX: examples ought to be up to the number of states in the fsm */
			char buf[50];
			int n;

			if (s == fsm->start) {
				ir->states[i].example = NULL;
				continue;
			}

			n = fsm_example(fsm, s, buf, sizeof buf);
			if (-1 == n) {
/* XXX:
perror("fsm_example");
*/
				return NULL;
			}

/* TODO:
			snprintf(buf, sizeof buf, "%s%s", buf,
				n >= (int) sizeof buf - 1 ? "..." : "");

			ir->states[i].example = strdup(buf);
*/

			ir->states[i].example = "TODO";
		}
	}

	/*
	 * Here we could merge struct ir_state entries if their
	 * opaque pointers compare equal by trivial shallow comparison.
	 *
	 * struct ir_state entries may be identical, because this is
	 * not necccessarily a minimal DFA. If this is a minimal DFA,
	 * then none of these should be equivalent.
	 */

	return ir;

error:

	/* TODO: free stuff */

	return NULL;
}

