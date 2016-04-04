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

#include "out.h"

static unsigned
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	unsigned int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 1; s != NULL; s = s->next, i++) {
		if (s == state) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

static void
escputc(int c, FILE *f)
{
	size_t i;

	struct {
		int in;
		const char *out;
	} esc[] = {
		{ FSM_EDGE_EPSILON, "&#x3B5;" },

		{ '&',  "&amp;"  },
		{ '\"', "&quot;" },
		{ '<',  "&#x3C;" },
		{ '>',  "&#x3E;" },

		{ '\\', "\\\\"   },
		{ '\f', "\\f"    },
		{ '\n', "\\n"    },
		{ '\r', "\\r"    },
		{ '\t', "\\t"    },
		{ '\v', "\\v"    },

		{ '^',  "\\^"    },
		{ '|',  "\\|"    },
		{ '[',  "\\["    },
		{ ']',  "\\]"    },
		{ '_',  "\\_"    },
		{ '-',  "\\-"    }
	};

	assert(f != NULL);

	for (i = 0; i < sizeof esc / sizeof *esc; i++) {
		if (esc[i].in == c) {
			fputs(esc[i].out, f);
			return;
		}
	}

	if (!isprint(c)) {
		fprintf(f, "\\x%x", (unsigned char) c);
		return;
	}

	putc(c, f);
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
			for (e = s->edges[i].sl; e != NULL; e = e->next) {
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
	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		for (e = s->edges[i].sl; e != NULL; e = e->next) {
			static const struct bm bm_empty;
			struct bm bm;
			int hi, lo;
			int k;
			int count; /* TODO: unsigned? */

			enum {
				MODE_INVERT,
				MODE_SINGLE,
				MODE_ANY,
				MODE_MANY
			} mode;

			assert(e->state != NULL);

			/* unique states only */
			if (contains(s->edges, i + 1, e->state)) {
				continue;
			}

			bm = bm_empty;

			/* find all edges which go from this state to the same target state */
			for (k = 0; k <= FSM_EDGE_MAX; k++) {
				if (set_contains(s->edges[k].sl, e->state)) {
					bm_set(&bm, k);
				}
			}

			count = bm_count(&bm);

			assert(count > 0);

			if (count == 1) {
				mode = MODE_SINGLE;
			} else if (bm_next(&bm, UCHAR_MAX, 1) != FSM_EDGE_MAX + 1) {
				mode = MODE_MANY;
			} else if (count == UCHAR_MAX + 1) {
				mode = MODE_ANY;
			} else if (count <= UCHAR_MAX / 2) {
				mode = MODE_MANY;
			} else {
				mode = MODE_INVERT;
			}

			if (mode == MODE_ANY) {
				fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = </./> ];\n",
					options->prefix != NULL ? options->prefix : "",
					indexof(fsm, s),
					options->prefix != NULL ? options->prefix : "",
					indexof(fsm, e->state));
				continue;
			}

			fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <%s%s",
				options->prefix != NULL ? options->prefix : "",
				indexof(fsm, s),
				options->prefix != NULL ? options->prefix : "",
				indexof(fsm, e->state),
				mode == MODE_SINGLE ? "" : "[",
				mode != MODE_INVERT ? "" : "^");

			/* now print the edges we found */
			hi = -1;
			for (;;) {
				/* start of range */
				lo = bm_next(&bm, hi, mode != MODE_INVERT);
				if (lo > UCHAR_MAX) {
					break;
				}

				/* end of range */
				hi = bm_next(&bm, lo, mode == MODE_INVERT);
				if (hi > UCHAR_MAX) {
					hi = UCHAR_MAX;
				}

				assert(hi > lo);

				switch (hi - lo) {
				case 1:
				case 2:
				case 3:
					escputc(lo, f);
					hi = lo;
					continue;

				default:
					escputc(lo, f);
					fprintf(f, "-");
					escputc(hi - 1, f);
					break;
				}
			}

			if (bm_get(&bm, FSM_EDGE_EPSILON)) {
				escputc(FSM_EDGE_EPSILON, f);
			}

			fprintf(f, "%s> ];\n", mode == MODE_SINGLE ? "" : "]");
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

