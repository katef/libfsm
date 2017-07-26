/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */

#include <adt/set.h>
#include <adt/bitmap.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/out.h>
#include <fsm/options.h>

#include "libfsm/out.h"

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

static int
escputc_hex(int c, FILE *f)
{
	assert(f != NULL);

	return fprintf(f, "\\x%02x", (unsigned char) c); /* for humans */
}

static int
escputc(int c, FILE *f)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ FSM_EDGE_EPSILON, "&#x3B5;" },

		{ '&',  "&amp;"    },
		{ '\"', "&quot;"   },
		{ ']',  "&#x5D;"   }, /* yes, even in a HTML label */
		{ '<',  "&#x3C;"   },
		{ '>',  "&#x3E;"   },
		{ ' ',  "&#x2423;" }
	};

	assert(f != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\x%02x", (unsigned char) c); /* for humans */
	}

	return fprintf(f, "%c", c);
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
singlestate(const struct fsm *fsm, FILE *f, struct fsm_state *s)
{
	struct fsm_edge *e, search;
	struct set_iter it;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(f != NULL);
	assert(s != NULL);

	if (!fsm->opt->anonymous_states) {
		fprintf(f, "\t%sS%-2u [ label = <",
			fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
			indexof(fsm, s));

		fprintf(f, "%u", indexof(fsm, s));

#ifdef DEBUG_TODFA
		if (s->nfasl != NULL) {
			struct set *q;

			assert(fsm->nfa != NULL);

			fprintf(f, "<br/>");

			fprintf(f, "{");

			for (q = s->nfasl; q != NULL; q = q->next) {
				fprintf(f, "%u", indexof(fsm->nfa, q->state));

				if (q->next != NULL) {
					fprintf(f, ",");
				}
			}

			fprintf(f, "}");
		}
#endif

		fprintf(f, "> ];\n");
	}

	if (!fsm->opt->consolidate_edges) {
		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			struct fsm_state *st;
			struct set_iter jt;

			for (st = set_first(e->sl, &jt); st != NULL; st = set_next(&jt)) {
				assert(st != NULL);

				fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
					fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
					indexof(fsm, s),
					fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
					indexof(fsm, st));

				if (fsm->opt->always_hex) {
					escputc_hex(e->symbol, f);
				} else {
					escputc(e->symbol, f);
				}

				fprintf(f, "> ];\n");
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

			fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				indexof(fsm, s),
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				indexof(fsm, st));

			(void) bm_print(f, &bm, 0,
				fsm->opt->always_hex ? escputc_hex: escputc);

			fprintf(f, "> ];\n");
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
			fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				indexof(fsm, s),
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				indexof(fsm, st));

			if (fsm->opt->always_hex) {
				escputc_hex(e->symbol, f);
			} else {
				escputc(e->symbol, f);
			}

			fprintf(f, "> ];\n");
		}
	}
}

static void
out_dotfrag(const struct fsm *fsm, FILE *f)
{
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(f != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		singlestate(fsm, f, s);

		if (fsm_isend(NULL, fsm, s)) {
			fprintf(f, "\t%sS%-2u [ shape = doublecircle ];\n",
				fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
				indexof(fsm, s));
		}
	}
}

void
fsm_out_dot(const struct fsm *fsm, FILE *f)
{
	struct fsm_state *start;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(f != NULL);

	if (fsm->opt->fragment) {
		out_dotfrag(fsm, f);
		return;
	}

	fprintf(f, "digraph G {\n");
	fprintf(f, "\trankdir = LR;\n");

	fprintf(f, "\tnode [ shape = circle ];\n");

	if (fsm->opt->anonymous_states) {
		fprintf(f, "\tnode [ label = \"\", width = 0.3 ];\n");
	}

	fprintf(f, "\n");

	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");

	start = fsm_getstart(fsm);
	fprintf(f, "\tstart -> %sS%u;\n",
		fsm->opt->prefix != NULL ? fsm->opt->prefix : "",
		 start != NULL ? indexof(fsm, start) : 0U);

	fprintf(f, "\n");

	out_dotfrag(fsm, f);

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

