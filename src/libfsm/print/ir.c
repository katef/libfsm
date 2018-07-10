/*
 * Copyright 2008-2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include "libfsm/internal.h"

#include "ir.h"

struct range {
	unsigned char start;
	unsigned char end;
	const struct fsm_state *to;
};

static int
range_cmp(const void *va, const void *vb)
{
	const struct range *a = va;
	const struct range *b = vb;

	assert(a != NULL);
	assert(b != NULL);

	if (a->to < b->to) { return -1; }
	if (a->to > b->to) { return +1; }

	/* TODO: could assert no overlapping (or adjacent!) ranges here */

	if (a->start < b->start) { return -1; }
	if (a->start > b->start) { return +1; }

	return 0;
}

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

static struct ir_group *
make_groups(const struct fsm *fsm, const struct fsm_state *state, const struct fsm_state *mode,
	size_t *u)
{
	struct range ranges[UCHAR_MAX]; /* worst case */
	struct ir_group *groups;
	struct fsm_edge *e;
	struct set_iter it;
	size_t i, j, k;
	size_t n;

	assert(fsm != NULL);
	assert(state != NULL);
	assert(u != NULL);

	/*
	 * The first pass here populates ranges[] by collating consecutive symbols
	 * which transition to the same FSM state. These ranges are constructed by
	 * iterating over symbols, and so are populated in symbol order.
	 * So several ranges may transition to the same state; these are grouped
	 * in the second pass below.
	 */

	n = 0;

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

		ranges[n].start = e->symbol;
		ranges[n].end   = e->symbol;
		ranges[n].to    = s;

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

				ranges[n].end = ne->symbol;

				e = set_next(&it);
			} while (e != NULL);
		}

		n++;
	}

	assert(n > 0);

	/*
	 * Now ranges have been identified, the second pass below groups them
	 * together by their common destination states.
	 */

	qsort(ranges, n, sizeof *ranges, range_cmp);

	groups = malloc(sizeof *groups * n); /* worst case */
	if (groups == NULL) {
		return NULL;
	}

	i = 0;
	j = 0;

	while (i < n) {
		const struct fsm_state *to;
		struct ir_range *p;

		to = ranges[i].to;

		p = malloc(sizeof *p * (n - i)); /* worst case */
		if (p == NULL) {
			j++;
			goto error;
		}

		k = 0;

		do {
			p[k].start = ranges[i].start;
			p[k].end   = ranges[i].end;

			i++;
			k++;
		} while (i < n && ranges[i].to == to);

		groups[j].to = indexof(fsm, to);
		groups[j].n = k;

		{
			void *tmp;

			tmp = realloc(p, sizeof *p * k);
			if (tmp == NULL) {
				j++;
				goto error;
			}

			p = tmp;
		}

		groups[j].ranges = p;

		j++;
	}

	*u = j;

	{
		void *tmp;

		tmp = realloc(groups, sizeof *groups * j);
		if (tmp == NULL) {
			goto error;
		}

		groups = tmp;
	}

	return groups;

error:

	for (i = 0; i < j; i++) {
		free((void *) groups[i].ranges);
	}

	free(groups);

	return NULL;
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
		cs->u.same.to = indexof(fsm, mode.state);
		return 0;
	}

	/* one dominant mode */
	if (mode.state != NULL) {
		cs->strategy = IR_DOMINANT;

		cs->u.dominant.groups = make_groups(fsm, state, mode.state, &cs->u.dominant.n);
		if (cs->u.dominant.groups == NULL) {
			return -1;
		}

		cs->u.dominant.mode = indexof(fsm, mode.state);
		return 0;
	}

	if (fsm_iscomplete(fsm, state)) {
		cs->strategy = IR_COMPLETE;

		cs->u.complete.groups = make_groups(fsm, state, NULL, &cs->u.complete.n);
		if (cs->u.complete.groups == NULL) {
			return -1;
		}

		return 0;
	}

	/* usual case */
	{
		cs->strategy = IR_PARTIAL;

		cs->u.partial.groups = make_groups(fsm, state, NULL, &cs->u.partial.n);
		if (cs->u.partial.groups == NULL) {
			return -1;
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
	assert(fsm->start != NULL);

	if (!fsm_all(fsm, fsm_isdfa)) {
		errno = EINVAL;
		return NULL;
	}

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
			ir->start = i;
		}

		if (make_state(fsm, s, ir, &ir->states[i]) == -1) {
			goto error;
		}

		if (!fsm->opt->comments) {
			ir->states[i].example = NULL;
		} else {
			char *p;
			int n;

			if (s == fsm->start) {
				ir->states[i].example = NULL;
				continue;
			}

			/*
			 * Example lengths are approximately proportional
			 * to the number of states in an fsm, and shorter where
			 * the graph branches often.
			 */
			p = malloc(ir->n + 3 + 1);
			if (p == NULL) {
				goto error;
			}

			n = fsm_example(fsm, s, p, ir->n + 1);
			if (-1 == n) {
				free(p);
				goto error;
			}

			if ((size_t) n < ir->n + 1) {
				char *tmp;

				n = strlen(p);

				tmp = realloc(p, n + 1);
				if (tmp == NULL) {
					free(p);
					goto error;
				}

				p = tmp;
			} else if ((size_t) n > ir->n + 1) {
				strcpy(p + ir->n, "...");
			}

			ir->states[i].example = p;
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

	ir->n = i;
	free_ir(ir);

	return NULL;
}

void
free_ir(struct ir *ir)
{
	size_t i;

	assert(ir != NULL);

	for (i = 0; i < ir->n; i++) {
		free((void *) ir->states[i].example);

		switch (ir->states[i].strategy) {
		case IR_TABLE:
			/* TODO */
			abort();

		case IR_NONE:
			break;

		case IR_SAME:
			break;

		case IR_COMPLETE:
			free((void *) ir->states[i].u.complete.groups);
			break;

		case IR_PARTIAL:
			free((void *) ir->states[i].u.partial.groups);
			break;

		case IR_DOMINANT:
			free((void *) ir->states[i].u.dominant.groups);
			break;
		}
	}

	free(ir->states);

	free(ir);
}

