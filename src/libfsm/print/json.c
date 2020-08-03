/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */

#include <print/esc.h>

#include <adt/set.h>
#include <adt/bitmap.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>

static void
print_edge_epsilon(FILE *f, int *notfirst,
	fsm_state_t src, fsm_state_t dst)
{
	assert(f != NULL);
	assert(notfirst != NULL);

	if (*notfirst) {
		fprintf(f, ",\n");
	}

	fprintf(f, "    { \"src\": %u, \"dst\": %u }", src, dst);

	*notfirst = 1;
}

static void
print_edge_symbol(FILE *f, int *notfirst, const struct fsm_options *opt,
	fsm_state_t src, fsm_state_t dst, unsigned char symbol)
{
	assert(f != NULL);
	assert(notfirst != NULL);

	if (*notfirst) {
		fprintf(f, ",\n");
	}

	/* "symbol" as opposed to "label" because this is a literal value */
	fprintf(f, "    { \"src\": %u, \"dst\": %u, \"symbol\": ",
		src, dst);

	if (opt->always_hex) {
		/* json doesn't have hex numeric literals, but this idea here is
		 * to output a numeric value rather than a string, so %u will do. */
		fprintf(f, "%u", symbol);
	} else {
		fprintf(f, "\"");
		json_escputc(f, opt, symbol);
		fprintf(f, "\"");
	}

	fprintf(f, " }");

	*notfirst = 1;
}

static void
print_edge_bitmap(FILE *f, int *notfirst, const struct fsm_options *opt,
	fsm_state_t src, fsm_state_t dst, const struct bm *bm)
{
	assert(f != NULL);
	assert(notfirst != NULL);
	assert(opt != NULL);
	assert(bm != NULL);

	if (*notfirst) {
		fprintf(f, ",\n");
	}

	fprintf(f, "    { \"src\": %u, \"dst\": %u, ", src, dst);

	/* "label" as opposed to "symbol" because this is a human-readable string */
	fprintf(f, "\"label\": \"");

	(void) bm_print(f, opt, bm, 0, json_escputc);

	fprintf(f, "\" }");

	*notfirst = 1;
}

static void
print_edge_label(FILE *f, int *notfirst,
	fsm_state_t src, fsm_state_t dst, const char *label)
{
	assert(f != NULL);
	assert(notfirst != NULL);
	assert(label != NULL);

	if (*notfirst) {
		fprintf(f, ",\n");
	}

	fprintf(f, "    { \"src\": %u, \"dst\": %u, \"label\": \"%s\" }",
		src, dst, label);

	*notfirst = 1;
}

static void
singlestate(FILE *f, const struct fsm *fsm, fsm_state_t s, int *notfirst)
{
	struct fsm_edge e;
	struct edge_iter it;
	struct edge_ordered_iter eoi;
	struct state_set *unique;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(s < fsm->statecount);
	assert(notfirst != NULL);

	if (!fsm->opt->consolidate_edges) {
		struct state_iter jt;
		fsm_state_t st;

		for (state_set_reset(fsm->states[s].epsilons, &jt); state_set_next(&jt, &st); ) {
			print_edge_epsilon(f, notfirst, s, st);
		}

		for (edge_set_ordered_iter_reset(fsm->states[s].edges, &eoi); edge_set_ordered_iter_next(&eoi, &e); ) {
			print_edge_symbol(f, notfirst, fsm->opt,
				s, e.state, e.symbol);
		}

		return;
	}

	unique = NULL;

	for (edge_set_reset(fsm->states[s].edges, &it); edge_set_next(&it, &e); ) {
		if (!state_set_add(&unique, fsm->opt->alloc, e.state)) {
			/* TODO: error */
			return;
		}
	}

	/*
	 * The consolidate_edges option is an aesthetic optimisation.
	 * For a state which has multiple edges all transitioning to the same state,
	 * all these edges are combined into a single edge, labelled with a more
	 * concise form of all their values.
	 *
	 * To implement this, we loop through all unique states, rather than
	 * looping through each edge.
	 */
	for (edge_set_ordered_iter_reset(fsm->states[s].edges, &eoi); edge_set_ordered_iter_next(&eoi, &e); ) {
		struct fsm_edge ne;
		struct edge_iter kt;
		struct bm bm;

		/* unique states only */
		if (!state_set_contains(unique, e.state)) {
			continue;
		}

		state_set_remove(&unique, e.state);

		bm_clear(&bm);

		/* find all edges which go from this state to the same target state */
		for (edge_set_reset(fsm->states[s].edges, &kt); edge_set_next(&kt, &ne); ) {
			if (ne.state == e.state) {
				bm_set(&bm, ne.symbol);
			}
		}

		print_edge_bitmap(f, notfirst, fsm->opt, s, e.state, &bm);
	}

	/*
	 * Special edges are not consolidated above
	 */
	{
		struct state_iter jt;
		fsm_state_t st;

		for (state_set_reset(fsm->states[s].epsilons, &jt); state_set_next(&jt, &st); ) {
			print_edge_label(f, notfirst, s, st, "\\u03B5");
		}
	}
}

void
fsm_print_json(FILE *f, const struct fsm *fsm)
{
	fsm_state_t start;
	fsm_state_t i;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	fprintf(f, "{\n");

	fprintf(f, "  \"statecount\": %u,\n", (unsigned) fsm->statecount);

	if (fsm_getstart(fsm, &start)) {
		fprintf(f, "  \"start\": %u,\n", start);
	}

	{
		int notfirst;

		notfirst = 0;

		fprintf(f, "  \"end\": [ ");
		for (i = 0; i < fsm->statecount; i++) {
			if (fsm_isend(fsm, i)) {
				if (notfirst) {
					fprintf(f, ", ");
				}

				fprintf(f, "%u", i);

				notfirst = 1;
			}
		}
		fprintf(f, " ],\n");
	}

	if (fsm->opt->endleaf != NULL) {
		int notfirst;

		notfirst = 0;

		fprintf(f, "  \"endleaf\": [ ");
		for (i = 0; i < fsm->statecount; i++) {
			if (fsm_isend(fsm, i)) {
				if (notfirst) {
					fprintf(f, ", ");
				}

				fprintf(f, "{ %u, ", i);

				fsm->opt->endleaf(f, &fsm->states[i], fsm->opt->endleaf_opaque);

				fprintf(f, " }");

				notfirst = 1;
			}
		}
		fprintf(f, " ],\n");
	}

	{
		int notfirst;

		notfirst = 0;

		fprintf(f, "  \"edges\": [\n");
			for (i = 0; i < fsm->statecount; i++) {
				singlestate(f, fsm, i, &notfirst);
			}
		fprintf(f, "\n  ]\n");
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

