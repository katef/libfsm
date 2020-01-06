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
singlestate(FILE *f, const struct fsm *fsm, fsm_state_t s)
{
	struct fsm_edge e;
	struct edge_iter it;
	struct state_set *unique;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(s < fsm->statecount);

	if (!fsm->opt->anonymous_states) {
		fprintf(f, "\t%sS%-2u [ label = <",
			fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
			s);

		fprintf(f, "%u", s);

		fprintf(f, "> ];\n");
	}

	if (!fsm->opt->consolidate_edges) {
		{
			struct state_iter jt;
			fsm_state_t st;

			for (state_set_reset(fsm->states[s].epsilons, &jt); state_set_next(&jt, &st); ) {
				fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
					fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
					s,
					fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
					st);

				fputs("&#x3B5;", f);

				fprintf(f, "> ];\n");
			}
		}

		for (edge_set_reset(fsm->states[s].edges, &it); edge_set_next(&it, &e); ) {
			fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				s,
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				e.state);

			dot_escputc_html(f, fsm->opt, e.symbol);

			fprintf(f, "> ];\n");
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
	for (edge_set_reset(fsm->states[s].edges, &it); edge_set_next(&it, &e); ) {
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

		fprintf(f, "\t%sS%-2u -> %sS%-2u [ ",
			fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
			s,
			fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
			e.state);

		if (bm_count(&bm) > 4) {
			fprintf(f, "weight = 3, ");
		}

		fprintf(f, "label = <");

		(void) bm_print(f, fsm->opt, &bm, 0, dot_escputc_html);

		fprintf(f, "> ];\n");
	}

	/*
	 * Special edges are not consolidated above
	 */
	{
		struct state_iter jt;
		fsm_state_t st;

		for (state_set_reset(fsm->states[s].epsilons, &jt); state_set_next(&jt, &st); ) {
			fprintf(f, "\t%sS%-2u -> %sS%-2u [ weight = 1, label = <",
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				s,
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				st);

			fputs("&#x3B5;", f);

			fprintf(f, "> ];\n");
		}
	}
}

static void
print_dotfrag(FILE *f, const struct fsm *fsm)
{
	fsm_state_t i;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		if (fsm_isend(fsm, i)) {
			fprintf(f, "\t%sS%-2u [ shape = doublecircle",
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "", i);

			if (fsm->opt->endleaf != NULL) {
				fprintf(f, ", ");
				fsm->opt->endleaf(f, &fsm->states[i], fsm->opt->endleaf_opaque);
			}

			fprintf(f, " ];\n");
		}

		/* TODO: show example here, unless !opt->comments */

		singlestate(f, fsm, i);
	}
}

void
fsm_print_dot(FILE *f, const struct fsm *fsm)
{
	fsm_state_t start;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	if (fsm->opt->fragment) {
		print_dotfrag(f, fsm);
		return;
	}

	fprintf(f, "digraph G {\n");
	fprintf(f, "\trankdir = LR;\n");

	fprintf(f, "\tnode [ shape = circle ];\n");
	fprintf(f, "\tedge [ weight = 2 ];\n");

	if (fsm->opt->anonymous_states) {
		fprintf(f, "\tnode [ label = \"\", width = 0.3 ];\n");
	}

	fprintf(f, "\troot = start;\n");
	fprintf(f, "\n");

	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");

	if (fsm_getstart(fsm, &start)) {
		fprintf(f, "\tstart -> %sS%u;\n",
			fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
			start);

		fprintf(f, "\n");
	}

	print_dotfrag(f, fsm);

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

