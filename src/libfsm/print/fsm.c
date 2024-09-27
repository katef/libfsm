/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */
#include "libfsm/print.h"

#include <print/esc.h>

#include <adt/set.h>
#include <adt/bitmap.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>
#include <adt/u64bitset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/options.h>

static int
default_accept(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);

	(void) hook_opaque;

	if (state_metadata->end_id_count == 0) {
		return 0;
	}

	fprintf(f, " = [");

	for (size_t i = 0; i < state_metadata->end_id_count; i++) {
		fprintf(f, "%u", state_metadata->end_ids[i]);

		if (i + 1 < state_metadata->end_id_count) {
			fprintf(f, ", ");
		}
	}

	fprintf(f, "]");

	return 0;
}

/* TODO: centralise */
static int
findany(const struct fsm *fsm, fsm_state_t state, fsm_state_t *a)
{
	struct fsm_edge e;
	struct edge_iter it;
	fsm_state_t f;
	struct bm bm;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	/*
	 * This approach is a little unsatisfying because it will only identify
	 * situations with one single "any" edge between states. I'd much prefer
	 * to be able to emit "any" edges for situations like:
	 *
	 *  1 -> 2 'x';
	 *  1 -> 3;
	 *
	 * where a given state also has an unrelated edge transitioning elsewhere.
	 * The current implementation conservatively bails out on that situation
	 * (because f != e.state), and will emit each edge separately.
	 */

	bm_clear(&bm);

	edge_set_reset(fsm->states[state].edges, &it);
	if (!edge_set_next(&it, &e)) {
		return 0;
	}

	/* if the first edge is not the first character,
	 * then we can't possibly have an "any" transition */
	if (e.symbol != '\0') {
		return 0;
	}

	f = e.state;

	for (edge_set_reset(fsm->states[state].edges, &it); edge_set_next(&it, &e); ) {
		if (f != e.state) {
			return 0;
		}

		/* we reject duplicate edges, even though they're to the same state */
		if (bm_get(&bm, e.symbol)) {
			return 0;
		}

		bm_set(&bm, e.symbol);
	}

	if (bm_count(&bm) != FSM_SIGMA_COUNT) {
		return 0;
	}

	assert(f < fsm->statecount);

	*a = f;
	return 1;
}

static int
print_state_comments(FILE *f,
	const struct fsm_options *opt,
	const struct fsm *fsm,
	fsm_state_t dst)
{
	fsm_state_t start;

	assert(f != NULL);
	assert(opt != NULL);
	assert(fsm != NULL);

	if (!fsm_getstart(fsm, &start)) {
		return 0;
	}

	if (dst == start) {
		fprintf(f, " # start");
	} else if (!fsm_has(fsm, fsm_hasepsilons)) {
		char buf[50];
		int n;

		n = fsm_example(fsm, dst, buf, sizeof buf);
		if (-1 == n) {
			return -1;
		}

		if (n > 0) {
			fprintf(f, " # e.g. \"");
			escputs(f, opt, fsm_escputc, buf);
			fprintf(f, "%s\"",
				n >= (int) sizeof buf - 1 ? "..." : "");
		}
	}

	return 0;
}

static void
print_char_range(FILE *f, const struct fsm_options *opt, char lower, char upper)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (lower == upper) {
		fprintf(f, "\"");
		fsm_escputc(f, opt, (char) lower);
		fprintf(f, "\"");
	} else {
		fprintf(f, "\"");
		fsm_escputc(f, opt, (char) lower);
		fprintf(f, "\" .. \"");
		fsm_escputc(f, opt, (char) upper);
		fprintf(f, "\"");
	}
}

static int
print_state(FILE *f, const struct fsm_options *opt, const struct fsm_hooks *hooks,
	const struct fsm *fsm,
	fsm_state_t s)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (!fsm_isend(fsm, s)) {
		if (-1 == print_hook_reject(f, opt, hooks, NULL, NULL)) {
			return -1;
		}
	}

	{
		struct state_iter jt;
		fsm_state_t st;

		for (state_set_reset(fsm->states[s].epsilons, &jt); state_set_next(&jt, &st); ) {
			fprintf(f, "%-2u -> %2u;\n", s, st);
		}
	}

	{
		fsm_state_t a;

		if (findany(fsm, s, &a)) {
			fprintf(f, "%-2u -> %2u ?;\n", s, a);
			return 0;
		}
	}

	assert(s < fsm->statecount);
	if (opt->group_edges) {
		struct edge_group_iter egi;
		struct edge_group_iter_info info;

		edge_set_group_iter_reset(fsm->states[s].edges,
		    EDGE_GROUP_ITER_ALL, &egi);
		while (edge_set_group_iter_next(&egi, &info)) {
			unsigned i, ranges = 0;
			unsigned lower = 256; /* start with lower bound out of range */
			assert(info.to < fsm->statecount);

			fprintf(f, "%-2u -> %2u ", s, info.to);

			for (i = 0; i < 256; i++) {
				if (u64bitset_get(info.symbols, i)) {
					if (lower > i) {
						lower = i; /* set lower bound for range */
					}
				} else {
					if (lower < i) {
						if (ranges > 0) {
							fprintf(f, ", ");
						}

						print_char_range(f, opt, (char) lower, (char) i - 1);
						ranges++;
					}
					lower = 256;
				}
			}
			if (lower < 256) { /* close last range */
				if (ranges > 0) {
					fprintf(f, ", ");
				}

				print_char_range(f, opt, (char) lower, (char) 255);
			}

			fprintf(f, ";");

			if (opt->comments) {
				if (-1 == print_state_comments(f, opt, fsm, info.to)) {
					return -1;
				}
			}

			fprintf(f, "\n");
		}
	} else {
		struct fsm_edge e;
		struct edge_ordered_iter eoi;

		for (edge_set_ordered_iter_reset(fsm->states[s].edges, &eoi);
		     edge_set_ordered_iter_next(&eoi, &e); ) {
			assert(e.state < fsm->statecount);

			fprintf(f, "%-2u -> %2u", s, e.state);

			fprintf(f, " \"");
			fsm_escputc(f, opt, (char) e.symbol);
			putc('\"', f);

			fprintf(f, ";");

			if (opt->comments) {
				if (-1 == print_state_comments(f, opt, fsm, e.state)) {
					return -1;
				}
			}

			fprintf(f, "\n");
		}
	}

	return 0;
}

int
fsm_print_fsm(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct fsm *fsm)
{
	fsm_state_t s, start;
	size_t end;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(fsm != NULL);

	if (!opt->anonymous_states) {
		/*
		 * States are output in order here so as to force ordering when
		 * parsing the .fsm format and creating new states. This ensures
		 * that the new states (numbered in order) match the numbering here.
		 */
		for (s = 0; s < fsm->statecount; s++) {
			fprintf(f, "%u;%s", s,
				s + 1 < fsm->statecount ? " " : "\n");
		}

		fprintf(f, "\n");
	}

	for (s = 0; s < fsm->statecount; s++) {
		if (-1 == print_state(f, opt, hooks, fsm, s)) {
			return -1;
		}
	}

	fprintf(f, "\n");

	if (!fsm_getstart(fsm, &start)) {
		goto done;
	}

	fprintf(f, "start: %u;\n", start);

	end = fsm->endcount;

	if (end == 0) {
		goto done;
	}

	fprintf(f, "end:   ");
	for (s = 0; s < fsm->statecount; s++) {
		fsm_end_id_t *ids;
		size_t count;

		if (!fsm_isend(fsm, s)) {
			continue;
		}

		end--;

		count = fsm_endid_count(fsm, s);
		if (count == 0) {
			ids = NULL;
		} else {
			int res;

			ids = f_malloc(fsm->alloc, count * sizeof *ids);
			if (ids == NULL) {
				return -1;
			}

			res = fsm_endid_get(fsm, s, count, ids);
			assert(res == 1);
		}

		fprintf(f, "%u", s);

		const struct fsm_state_metadata state_metadata = {
			.end_ids = ids,
			.end_id_count = count,
		};

		if (-1 == print_hook_accept(f, opt, hooks,
			&state_metadata,
			default_accept,
			NULL))
		{
			return -1;
		}

		if (-1 == print_hook_comment(f, opt, hooks,
			&state_metadata))
		{
			return -1;
		}

		if (count > 0) {
			f_free(fsm->alloc, ids);
		}

		fprintf(f, "%s", end > 0 ? ", " : ";\n");
	}

done:

	return 0;
}

