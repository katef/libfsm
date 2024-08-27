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

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include <print/esc.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/bitmap.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "libfsm/internal.h"
#include "libfsm/eager_output.h"

#include "ir.h"

/* ranges are inclusive */
struct range {
	unsigned char start;
	unsigned char end;
	fsm_state_t to;
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

static struct ir_group *
make_groups(const struct fsm *fsm, fsm_state_t state,
	fsm_state_t mode, int have_mode,
	size_t *u)
{
	struct range ranges[FSM_SIGMA_COUNT]; /* worst case, one per symbol */
	struct ir_group *groups;
	struct fsm_edge e;
	struct edge_ordered_iter eoi;
	size_t i, j, k;
	size_t grp_ind, n;

	assert(fsm != NULL);
	assert(state < fsm->statecount);
	assert(u != NULL);

	/* The first pass gathers ranges, and opportunistically combines
	 * adjacent ranges with consecutive symbols which transition to
	 * the same FSM state. Since the edges are stored unordered (in
	 * a hash table), a follow-up pass sorts and then combines any
	 * newly adjacent ranges to the same state. */

	n = 0;          /* number of allocated ranges */
	grp_ind = 0;    /* index of current working range */

	for (edge_set_ordered_iter_reset(fsm->states[state].edges, &eoi); edge_set_ordered_iter_next(&eoi, &e); ) {
		if (have_mode && e.state == mode) {
			continue;
		}

		if (n > 0 && e.symbol == ranges[grp_ind].end + 1 && e.state == ranges[grp_ind].to) {
			ranges[grp_ind].end = e.symbol;
		} else {
			grp_ind = n++;

			ranges[grp_ind].start = e.symbol;
			ranges[grp_ind].end   = e.symbol;
			ranges[grp_ind].to    = e.state;
		}
	}

	assert(n > 0);

	/* Sort and combine consecutive states. */
	qsort(ranges, n, sizeof *ranges, range_cmp);
	{
		size_t src, dst = 0;
		for (src = 1; src < n; src++) {
			if (ranges[src].to == ranges[dst].to
			    && ranges[src].start == ranges[dst].end + 1) {
				ranges[dst].end = ranges[src].end;
			} else {
				assert(dst < src);
				dst++;
				memcpy(&ranges[dst], &ranges[src],
				    sizeof(ranges[dst]));
			}
		}
		n = dst + 1;
	}

	/*
	 * Now ranges have been identified, the next pass below groups them
	 * together by their common destination states.
	 */

	groups = f_malloc(fsm->alloc, sizeof *groups * n); /* worst case */
	if (groups == NULL) {
		return NULL;
	}

	i = 0;
	j = 0;

	while (i < n) {
		struct ir_range *p;
		fsm_state_t to;

		to = ranges[i].to;

		p = f_malloc(fsm->alloc, sizeof *p * (n - i)); /* worst case */
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

		groups[j].to = to;
		groups[j].n = k;

		{
			void *tmp;

			tmp = f_realloc(fsm->alloc, p, sizeof *p * k);
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

		tmp = f_realloc(fsm->alloc, groups, sizeof *groups * j);
		if (tmp == NULL) {
			goto error;
		}

		groups = tmp;
	}

	return groups;

error:

	for (i = 0; i < j; i++) {
		f_free(fsm->alloc, (void *) groups[i].ranges);
	}

	f_free(fsm->alloc, groups);

	return NULL;
}

static void
free_groups(const struct fsm *fsm, struct ir_group *groups, size_t n)
{
	size_t j;

	for (j = 0; j < n; j++) {
		f_free(fsm->alloc, (void *) groups[j].ranges);
	}

	f_free(fsm->alloc, groups);
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
make_holes(const struct fsm *fsm, const struct bm *bm, size_t *n)
{
	struct ir_range *ranges;
	int hi, lo;

	assert(bm != NULL);
	assert(n != NULL);

	ranges = f_malloc(fsm->alloc, sizeof *ranges * FSM_SIGMA_COUNT); /* worst case */
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

		/* one past the end of range */
		hi = bm_next(bm, lo, 1);

		ranges[*n].start = lo;
		ranges[*n].end   = hi - 1;

		(*n)++;
	}

	{
		void *tmp;

		tmp = f_realloc(fsm->alloc, ranges, sizeof *ranges * *n);
		if (tmp == NULL) {
			goto error;
		}

		ranges = tmp;
	}

	return ranges;

error:

	f_free(fsm->alloc, ranges);

	return NULL;
}

static int
make_state(const struct fsm *fsm, fsm_state_t state,
	struct ir_state *cs)
{
	struct ir_group *groups;
	struct group_count max;
	struct bm bm;
	size_t hole;
	size_t n;

	struct {
		fsm_state_t state;
		unsigned int freq; /* 0 meaning no mode */
	} mode;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	/* TODO: IR_TABLE */

	/* no edges */
	{
		if (edge_set_empty(fsm->states[state].edges)) {
			cs->strategy = IR_NONE;
			return 0;
		}
	}

	if (fsm_iscomplete(fsm, state)) {
		mode.state = fsm_findmode(fsm, state, &mode.freq);
	} else {
		mode.state = 0; /* XXX: dummy */
		mode.freq = 0;
	}

	assert(mode.freq == 0 || mode.state < fsm->statecount);

	/* all edges go to the same state */
	if (mode.freq == FSM_SIGMA_COUNT) {
		cs->strategy  = IR_SAME;
		cs->u.same.to = mode.state;
		return 0;
	}

	groups = make_groups(fsm, state, mode.state, mode.freq > 0, &n);
	if (groups == NULL) {
		return -1;
	}

	/* one dominant mode */
	if (mode.freq > 0) {
		cs->strategy = IR_DOMINANT;
		cs->u.dominant.groups = groups;
		cs->u.dominant.n = n;
		cs->u.dominant.mode = mode.state;
		return 0;
	}

	find_coverage(groups, n, &bm);

	hole = UCHAR_MAX + 1 - bm_count(&bm);

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

		f_free(fsm->alloc, (void *) groups[max.j].ranges);
		if (max.j < n) {
			memmove(groups + max.j, groups + max.j + 1, sizeof *groups * (n - max.j - 1));
			n--;
			/* don't bother reallocing down; it's just unneccessary overhead */
		}

		cs->u.error.error.ranges = make_holes(fsm, &bm, &cs->u.error.error.n);
		if (cs->u.error.error.ranges == NULL) {
			free_groups(fsm, groups, n);
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

int
make_example(const struct fsm *fsm, fsm_state_t s, char **example)
{
	fsm_state_t start;
	size_t count;
	char *p;
	int n;

	assert(fsm != NULL);
	assert(example != NULL);

	if (!fsm_getstart(fsm, &start)) {
		*example = NULL;
		return 1;
	}

	count = fsm_countstates(fsm);

	/*
	 * Example lengths are approximately proportional to the number of states,
	 * and shorter where the graph branches often.
	 */
	p = f_malloc(fsm->alloc, count + 3 + 1);
	if (p == NULL) {
		return 0;
	}

	/*
	 * fsm_example() doesn't distinguish an unreachable state from n = 0
	 */
	if (s == start) {
		p[0] = '\0';
		goto done;
	}

	n = fsm_example(fsm, s, p, count + 1);
	if (-1 == n) {
		goto error;
	}

	/*
	 * The goal state is always reachable for a DFA with
	 * no "stray" states, but the caller may not trim an FSM,
	 * and need not pass a DFA.
	 */
	if (n == 0) {
		f_free(fsm->alloc, p);
		p = NULL;
	} else if ((size_t) n < count + 1) {
		char *tmp;

		tmp = f_realloc(fsm->alloc, p, strlen(p) + 1);
		if (tmp == NULL) {
			goto error;
		}

		p = tmp;
	} else if ((size_t) n > count + 1) {
		strcpy(p + count, "...");
	}

done:

	*example = p;

	return 1;

error:

	f_free(fsm->alloc, p);

	return 0;
}

static int
append_eager_output_cb(fsm_state_t state, fsm_output_id_t id, void *opaque)
{
	struct ir_state_eager_output *outputs = opaque;
	(void)state;
	outputs->ids[outputs->count++] = id;
	return 1;
}

static int
cmp_fsm_output_id_t(const void *pa, const void *pb)
{
	const fsm_output_id_t a = *(fsm_output_id_t *)pa;
	const fsm_output_id_t b = *(fsm_output_id_t *)pb;
	return a < b ? -1 : a > b ? 1 : 0;
}

struct ir *
make_ir(const struct fsm *fsm, const struct fsm_options *opt)
{
	fsm_state_t start, i;
	struct ir *ir;

	assert(fsm != NULL);
	assert(opt != NULL);

	if (!fsm_getstart(fsm, &start)) {
		goto empty;
	}

	if (!fsm_all(fsm, fsm_isdfa)) {
		errno = EINVAL;
		return NULL;
	}

	ir = f_malloc(fsm->alloc, sizeof *ir);
	if (ir == NULL) {
		return NULL;
	}

	ir->n      = fsm_countstates(fsm);
	ir->states = f_malloc(fsm->alloc, sizeof *ir->states * ir->n);
	if (ir->states == NULL) {
		f_free(fsm->alloc, ir);
		return NULL;
	}

	ir->start = start;

	for (i = 0; i < fsm->statecount; i++) {
		assert(i < ir->n);

		ir->states[i].isend  = fsm_isend(fsm, i);
		ir->states[i].endids.ids = NULL;
		ir->states[i].endids.count = 0;

		ir->states[i].eager_outputs = NULL;

		if (fsm_isend(fsm, i)) {
			fsm_end_id_t *ids;
			size_t count;

			count = fsm_endid_count(fsm, i);
			if (count == 0) {
				ids = NULL;
			} else {
				int res;

				ids = f_malloc(fsm->alloc, count * sizeof *ids);
				if (ids == NULL) {
					goto error;
				}

				res = fsm_endid_get(fsm, i, count, ids);
				assert(res == 1);
			}

			ir->states[i].endids.ids = ids;
			ir->states[i].endids.count = count;
		}

		size_t count;
		if (fsm_eager_output_has_any(fsm, i, &count)) {
			struct ir_state_eager_output *outputs = f_malloc(fsm->alloc,
			    sizeof(*outputs) + count * sizeof(outputs->ids[0]));
			if (outputs == NULL) {
				goto error;
			}
			outputs->count = 0;
			fsm_eager_output_iter_state(fsm, i, append_eager_output_cb, outputs);
			assert(outputs->count == count);
			qsort(outputs->ids, outputs->count, sizeof(outputs->ids[0]), cmp_fsm_output_id_t);
			ir->states[i].eager_outputs = outputs;
		}

		if (make_state(fsm, i, &ir->states[i]) == -1) {
			goto error;
		}

		if (!opt->comments) {
			ir->states[i].example = NULL;
		} else {
			char *example;

			if (!make_example(fsm, i, &example)) {
				goto error_example;
			}

			ir->states[i].example = example;
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

empty:

	ir = f_malloc(fsm->alloc, sizeof *ir);
	if (ir == NULL) {
		return NULL;
	}

	ir->n      = 0;
	ir->start  = 0;
	ir->states = NULL;

	return ir;

error_example:

	i++;

error:

	ir->n = i;
	free_ir(fsm, ir);

	return NULL;
}

void
free_ir(const struct fsm *fsm, struct ir *ir)
{
	size_t i;

	assert(ir != NULL);

	for (i = 0; i < ir->n; i++) {
		f_free(fsm->alloc, (void *) ir->states[i].example);
		f_free(fsm->alloc, (void *) ir->states[i].endids.ids);
		f_free(fsm->alloc, (void *) ir->states[i].eager_outputs);

		switch (ir->states[i].strategy) {
		case IR_TABLE:
			/* TODO */
			abort();

		case IR_NONE:
			break;

		case IR_SAME:
			break;

		case IR_COMPLETE:
			free_groups(fsm, (void *) ir->states[i].u.complete.groups,
				ir->states[i].u.complete.n);
			break;

		case IR_PARTIAL:
			free_groups(fsm, (void *) ir->states[i].u.partial.groups,
				ir->states[i].u.partial.n);
			break;

		case IR_DOMINANT:
			free_groups(fsm, (void *) ir->states[i].u.dominant.groups,
				ir->states[i].u.dominant.n);
			break;

		case IR_ERROR:
			f_free(fsm->alloc, (void *) ir->states[i].u.error.error.ranges);
			free_groups(fsm, (void *) ir->states[i].u.error.groups,
				ir->states[i].u.error.n);
			break;
		}
	}

	f_free(fsm->alloc, ir->states);

	f_free(fsm->alloc, ir);
}

