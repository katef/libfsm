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

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>

static unsigned
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

/* Return true if the edges after o contains state */
/* TODO: centralise */
static int
contains(struct set *edges, int o, struct fsm_state *state)
{
	struct fsm_edge *e, search;
	struct set_iter it;

	assert(edges != NULL);
	assert(state != NULL);

	search.symbol = o;
	for (e = set_firstafter(edges, &it, &search); e != NULL; e = set_next(&it)) {
		if (e->symbol > UCHAR_MAX) {
			return 0;
		}

		if (set_contains(e->sl, state)) {
			return 1;
		}
	}

	return 0;
}

static void
singlestate(FILE *f, const struct fsm *fsm, struct fsm_state *s)
{
	struct fsm_edge *e, search;
	struct set_iter it;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(s != NULL);

	if (fsm->opt->anonymous_states) {
		fprintf(f, "\tnode: { title:\"%u\" label: \" \" shape: ellipse }\n",
			indexof(fsm, s));
	} else {
		fprintf(f, "\tnode: { title:\"%u\" shape: ellipse }\n",
			indexof(fsm, s));
	}

#if 0
		if (fsm_isend(fsm, s)) {
			fprintf(f, "\t%s%u [ shape = doublecircle ];\n",
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				indexof(fsm, s));
		}
#endif

	if (!fsm->opt->consolidate_edges) {
		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			struct fsm_state *st;
			struct set_iter jt;

			for (st = set_first(e->sl, &jt); st != NULL; st = set_next(&jt)) {
				assert(st != NULL);

				fprintf(f, "\tedge: { sourcename: \"%u\" targetname: \"%u\" label = \"",
					indexof(fsm, s),
					indexof(fsm, st));

				if (e->symbol <= UCHAR_MAX) {
					dot_escputc_html(f, fsm->opt, e->symbol);
				} else if (e->symbol == FSM_EDGE_EPSILON) {
					fputs("&#x3B5;", f);
				} else {
					assert(!"unrecognised special edge");
					abort();
				}

				fprintf(f, "\" }\n");
			}
		}

		return;
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
	/* TODO: handle special edges upto FSM_EDGE_MAX separately */
	for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
		struct fsm_state *st;
		struct set_iter jt;

		if (e->symbol > UCHAR_MAX) {
			break;
		}

		for (st = set_first(e->sl, &jt); st != NULL; st = set_next(&jt)) {
			struct fsm_edge *ne;
			struct set_iter kt;
			struct bm bm;

			assert(st != NULL);

			/* unique states only */
			if (contains(s->edges, e->symbol, st)) {
				continue;
			}

			bm_clear(&bm);

			/* find all edges which go from this state to the same target state */
			for (ne = set_first(s->edges, &kt); ne != NULL; ne = set_next(&kt)) {
				if (ne->symbol > UCHAR_MAX) {
					break;
				}

				if (set_contains(ne->sl, st)) {
					bm_set(&bm, ne->symbol);
				}
			}

			fprintf(f, "\tedge: { sourcename: \"%u\" targetname: \"%u\" ",
				indexof(fsm, s),
				indexof(fsm, st));

#if 0
			if (bm_count(&bm) > 4) {
				fprintf(f, "weight = 3, ");
			}
#endif

			fprintf(f, "label: \"");

			(void) bm_print(f, fsm->opt, &bm, 0, dot_escputc_html);

			fprintf(f, "\" }\n");
		}
	}

	/*
	 * Special edges are not consolidated above
	 */
	search.symbol = UCHAR_MAX;
	for (e = set_firstafter(s->edges, &it, &search); e != NULL; e = set_next(&it)) {
		struct fsm_state *st;
		struct set_iter jt;

		for (st = set_first(e->sl, &jt); st != NULL; st = set_next(&jt)) {
			fprintf(f, "\tedge: { sourcename: \"%u\" targetname: \"%u\" label: \"",
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				indexof(fsm, s),
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				indexof(fsm, st));

			assert(e->symbol > UCHAR_MAX);
			if (e->symbol == FSM_EDGE_EPSILON) {
				fputs("&#x3B5;", f);
			} else {
				assert(!"unrecognised special edge");
				abort();
			}

			fprintf(f, "\" }\n");
		}
	}
}

static void
print_vcgfrag(FILE *f, const struct fsm *fsm)
{
	struct fsm_state *s;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		singlestate(f, fsm, s);
	}
}

void
fsm_print_vcg(FILE *f, const struct fsm *fsm)
{
	struct fsm_state *start;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	if (fsm->opt->fragment) {
		print_vcgfrag(f, fsm);
		return;
	}

	fprintf(f, "graph: {\n");

	fprintf(f, "\tlayoutalgorithm: minbackward\n");
	fprintf(f, "\tdisplay_edge_labels: yes\n");
	fprintf(f, "\tsplines: yes\n");
	fprintf(f, "\tsplinefactor: 2\n");
	fprintf(f, "\torientation: left_to_right\n");
	fprintf(f, "\tnode_alignment: center\n");
	fprintf(f, "\tdirty_edge_labels: yes\n");
	fprintf(f, "\n");

#if 0
	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");

	start = fsm_getstart(fsm);
	fprintf(f, "\tstart -> %s%u;\n",
		fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
		 start != NULL ? indexof(fsm, start) : 0U);

	fprintf(f, "\n");
#endif

	print_vcgfrag(f, fsm);

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

