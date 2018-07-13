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

#include <print/esc.h>

#include <adt/set.h>
#include <adt/bitmap.h>

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

struct group_count {
	size_t j;
	size_t n;
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

static void
free_groups(struct ir_group *groups, size_t n)
{
	size_t j;

	for (j = 0; j < n; j++) {
		free((void *) groups[j].ranges);
	}

	free(groups);
}

static void
find_coverage(const struct ir_group *groups, size_t n, struct bm *bm)
{
	size_t j, i;
	unsigned int e;

	assert(groups != NULL);
	assert(bm != NULL);

	bm_clear(bm);

	for (j = 0; j < n; j++) {
		for (i = 0; i < groups[j].n; i++) {
			assert(groups[j].ranges[i].end >= groups[j].ranges[i].start);
			for (e = groups[j].ranges[i].start; e <= groups[j].ranges[i].end; e++) {
				assert(!bm_get(bm, e));
				bm_set(bm, e);
			}
		}
	}
}

static struct group_count
group_max(const struct ir_group *groups, size_t n)
{
	struct group_count curr;
	size_t j, i;

	assert(groups != NULL);

	curr.n = 0;

	for (j = 0; j < n; j++) {
		size_t n;

		n = 0;

		for (i = 0; i < groups[j].n; i++) {
			assert(groups[j].ranges[i].end >= groups[j].ranges[i].start);
			n += groups[j].ranges[i].end - groups[j].ranges[i].start;
		}

		if (n > curr.n) {
			curr.n = n;
			curr.j = j;
		}
	}

	return curr;
}

static struct ir_range *
make_holes(const struct bm *bm, size_t *n)
{
	struct ir_range *ranges;
	int hi, lo;

	assert(bm != NULL);
	assert(n != NULL);

	ranges = malloc(sizeof *ranges * UCHAR_MAX); /* worst case */
	if (ranges == NULL) {
		return NULL;
	}

	hi = -1;
	*n = 0;

	for (;;) {
		/* start of range */
		lo = bm_next(bm, hi, 0);
		if (lo > UCHAR_MAX) {
			break;
		}

		/* end of range */
		hi = bm_next(bm, lo, 1);

		ranges[*n].start = lo;
		ranges[*n].end   = hi - 1;

		(*n)++;
	}

	{
		void *tmp;

		tmp = realloc(ranges, sizeof *ranges * *n);
		if (tmp == NULL) {
			goto error;
		}

		ranges = tmp;
	}

	return ranges;

error:

	free(ranges);

	return NULL;
}

static int
make_state(const struct fsm *fsm,
	struct fsm_state *state,
	const struct ir *ir, struct ir_state *cs)
{
	struct ir_group *groups;
	struct group_count max;
	struct bm bm;
	size_t hole;
	size_t n;

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

	groups = make_groups(fsm, state, mode.state, &n);
	if (groups == NULL) {
		return -1;
	}

	/* one dominant mode */
	if (mode.state != NULL) {
		cs->strategy = IR_DOMINANT;
		cs->u.dominant.groups = groups;
		cs->u.dominant.n = n;
		cs->u.dominant.mode = indexof(fsm, mode.state);
		return 0;
	}

	find_coverage(groups, n, &bm);

	hole = UCHAR_MAX - bm_count(&bm);

	if (hole == 0) {
		assert(fsm_iscomplete(fsm, state));
		cs->strategy = IR_COMPLETE;
		cs->u.complete.groups = groups;
		cs->u.complete.n = n;
		return 0;
	}

	max = group_max(groups, n);

	/*
	 * This isn't always best choice (the worst case is where the hole
	 * is interlaced over a large region, so the code generation comes
	 * out larger). But it's a reasonable attempt for typical regexps,
	 * where holes tend to be mostly contigious.
	 */
	if (hole < max.n) {
		cs->strategy = IR_ERROR;
		cs->u.error.mode = groups[max.j].to;

		free((void *) groups[max.j].ranges);
		if (max.j < n) {
			memmove(groups + max.j, groups + max.j + 1, sizeof *groups * (n - max.j - 1));
			n--;
			/* don't bother reallocing down; it's just unneccessary overhead */
		}

		cs->u.error.error.ranges = make_holes(&bm, &cs->u.error.error.n);
		if (cs->u.error.error.ranges == NULL) {
			free_groups(groups, n);
			return -1;
		}

		cs->u.error.groups = groups;
		cs->u.error.n = n;
		return 0;
	}


	/* usual case */
	{
		cs->strategy = IR_PARTIAL;

		cs->u.partial.groups = groups;
		cs->u.partial.n = n;
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
				goto error_example;
			}

			n = fsm_example(fsm, s, p, ir->n + 1);
			if (-1 == n) {
				free(p);
				goto error_example;
			}

			if ((size_t) n < ir->n + 1) {
				char *tmp;

				n = strlen(p);

				tmp = realloc(p, n + 1);
				if (tmp == NULL) {
					free(p);
					goto error_example;
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

error_example:

	ir->states[i].example = NULL;
	i++;

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
			free_groups((void *) ir->states[i].u.complete.groups,
				ir->states[i].u.complete.n);
			break;

		case IR_PARTIAL:
			free_groups((void *) ir->states[i].u.partial.groups,
				ir->states[i].u.partial.n);
			break;

		case IR_DOMINANT:
			free_groups((void *) ir->states[i].u.dominant.groups,
				ir->states[i].u.dominant.n);
			break;

		case IR_ERROR:
			free((void *) ir->states[i].u.error.error.ranges);
			free_groups((void *) ir->states[i].u.error.groups,
				ir->states[i].u.error.n);
			break;
		}
	}

	free(ir->states);

	free(ir);
}

