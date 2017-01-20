/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */

#include <adt/set.h>
#include <adt/bitmap.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

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
escputc(int c, FILE *f)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ FSM_EDGE_EPSILON, "&#x3B5;" },

		{ '&',  "&amp;"  },
		{ '\"', "&quot;" },
		{ ']',  "&#x5D;" }, /* yes, even in a HTML label */
		{ '<',  "&#x3C;" },
		{ '>',  "&#x3E;" }
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
static int
contains(struct fsm_edge edges[], int o, struct fsm_state *state)
{
	int i;

	assert(edges != NULL);
	assert(state != NULL);

	for (i = o; i <= FSM_EDGE_MAX; i++) {
		if (set_contains(edges[i].sl, state)) {
			return 1;
		}
	}

	return 0;
}

static void
singlestate(const struct fsm *fsm, FILE *f, struct fsm_state *s,
	const struct fsm_outoptions *options)
{
	struct set_iter iter;
	struct state_set *e;
	int i;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(s != NULL);
	assert(options != NULL);

	if (!options->anonymous_states) {
		fprintf(f, "\t%sS%-2u [ label = <",
			options->prefix != NULL ? options->prefix : "",
			indexof(fsm, s));

		fprintf(f, "%u", indexof(fsm, s));

#ifdef DEBUG_TODFA
		if (s->nfasl != NULL) {
			struct state_set *q;

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

	if (!options->consolidate_edges) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (e = set_first(s->edges[i].sl, &iter); e != NULL; e = set_next(&iter)) {
				assert(e->state != NULL);

				fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
					options->prefix != NULL ? options->prefix : "",
					indexof(fsm, s),
					options->prefix != NULL ? options->prefix : "",
					indexof(fsm, e->state));

				escputc(i, f);

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
	for (i = 0; i <= UCHAR_MAX; i++) {
		for (e = set_first(s->edges[i].sl, &iter); e != NULL; e = set_next(&iter)) {
			struct bm bm;
			int k;

			assert(e->state != NULL);

			/* unique states only */
			if (contains(s->edges, i + 1, e->state)) {
				continue;
			}

			bm_clear(&bm);

			/* find all edges which go from this state to the same target state */
			for (k = 0; k <= UCHAR_MAX; k++) {
				if (set_contains(s->edges[k].sl, e->state)) {
					bm_set(&bm, k);
				}
			}

			fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
				options->prefix != NULL ? options->prefix : "",
				indexof(fsm, s),
				options->prefix != NULL ? options->prefix : "",
				indexof(fsm, e->state));

			(void) bm_print(f, &bm, 0, escputc);

			fprintf(f, "> ];\n");
		}
	}

	/*
	 * Special edges are not consolidated above
	 */
	for (i = UCHAR_MAX; i <= FSM_EDGE_MAX; i++) {
		for (e = set_first(s->edges[i].sl, &iter); e != NULL; e = set_next(&iter)) {
			fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
				options->prefix != NULL ? options->prefix : "",
				indexof(fsm, s),
				options->prefix != NULL ? options->prefix : "",
				indexof(fsm, e->state));

			escputc(i, f);

			fprintf(f, "> ];\n");
		}
	}
}

static void
out_dotfrag(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options)
{
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		singlestate(fsm, f, s, options);

		if (fsm_isend(fsm, s)) {
			fprintf(f, "\t%sS%-2u [ shape = doublecircle ];\n",
				options->prefix != NULL ? options->prefix : "",
				indexof(fsm, s));
		}
	}
}

void
fsm_out_dot(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options)
{
	struct fsm_state *start;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	if (options->fragment) {
		out_dotfrag(fsm, f, options);
		return;
	}

	fprintf(f, "digraph G {\n");
	fprintf(f, "\trankdir = LR;\n");

	fprintf(f, "\tnode [ shape = circle ];\n");

	if (options->anonymous_states) {
		fprintf(f, "\tnode [ label = \"\", width = 0.3 ];\n");
	}

	fprintf(f, "\n");

	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");

	start = fsm_getstart(fsm);
	fprintf(f, "\tstart -> %sS%u;\n",
		options->prefix != NULL ? options->prefix : "",
		 start != NULL ? indexof(fsm, start) : 0U);

	fprintf(f, "\n");

	out_dotfrag(fsm, f, options);

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

