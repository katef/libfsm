/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include <fsm/fsm.h>

#include "out.h"
#include "libfsm/internal.h"
#include "libfsm/set.h"
#include "libfsm/colour.h"

struct bm {
	unsigned char map[FSM_EDGE_MAX / CHAR_BIT + 1];
};

static int bm_get(struct bm *bm, size_t i) {
	assert(bm != NULL);
	assert(i <= FSM_EDGE_MAX);

	return bm->map[i / CHAR_BIT] & (1 << i % CHAR_BIT);
}

static void bm_set(struct bm *bm, size_t i) {
	assert(bm != NULL);
	assert(i <= FSM_EDGE_MAX);

	bm->map[i / CHAR_BIT] |=  (1 << i % CHAR_BIT);
}

size_t
bm_next(const struct bm *bm, int i, int value)
{
	size_t n;

	assert(bm != NULL);
	assert(i / CHAR_BIT < FSM_EDGE_MAX);

	/* this could be faster by incrementing per element instead of per bit */
	for (n = i + 1; n <= FSM_EDGE_MAX; n++) {
		/* ...and this could be faster by using peter wegner's method */
		if (!(bm->map[n / CHAR_BIT] & (1 << n % CHAR_BIT)) == !value) {
			return n;
		}
	}

	return FSM_EDGE_MAX + 1;
}

static unsigned int
bm_single(const struct bm *bm)
{
	size_t i;

	assert(bm != NULL);

	i = bm_next(bm, -1, 1);
	i = bm_next(bm,  i, 1);

	return i == FSM_EDGE_MAX + 1;
}

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

static void escputc(int c, FILE *f) {
	size_t i;

	struct {
		int in;
		const char *out;
	} esc[] = {
		{ FSM_EDGE_EPSILON, "&#x3B5;" },

		{ '&',  "&amp;"  },
		{ '\"', "&quot;" },
		{ '\\', "\\\\"   },
		{ '\n', "\\n"    },
		{ '\r', "\\r"    },
		{ '\f', "\\f"    },
		{ '\v', "\\v"    },
		{ '\t', "\\t"    },

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
		fprintf(f, "0x%x", (unsigned char) c);
		return;
	}

	putc(c, f);
}

static void escputs(const char *s, FILE *f) {
	const char *p;

	assert(f != NULL);
	assert(s != NULL);

	for (p = s; *p != '\0'; p++) {
		escputc(*p, f);
	}
}

/* Return true if the edges after o contains state */
static int contains(struct fsm_edge edges[], int o, struct fsm_state *state) {
	int i;

	assert(edges != NULL);
	assert(state != NULL);

	for (i = o; i <= FSM_EDGE_MAX; i++) {
		if (set_contains(state, edges[i].sl)) {
			return 1;
		}
	}

	return 0;
}

static void printcolours(const struct fsm *fsm, const struct colour_set *cl, FILE *f) {
	const struct colour_set *c;

	assert(fsm != NULL);
	assert(f != NULL);

	if (cl == NULL) {
		return;
	}

	if (fsm->colour_hooks.print == NULL) {
		return;
	}

	fprintf(f, "<font point-size = \"12\">");

	for (c = cl; c != NULL; c = c->next) {
		fprintf(f, "<font color = \"");

		fsm->colour_hooks.print(fsm, f, c->colour);

		fprintf(f, "\">");

		/* TODO: html escapes */
		fsm->colour_hooks.print(fsm, f, c->colour);

		fprintf(f, "</font>");

		if (c->next != NULL) {
			fprintf(f, ",");
		}
	}

	fprintf(f, "</font>");
}

static void singlestate(const struct fsm *fsm, FILE *f, struct fsm_state *s, const struct fsm_outoptions *options) {
	struct state_set *e;
	int i;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(s != NULL);
	assert(options != NULL);

	/* TODO: findany() here? */
	if (fsm_isend(fsm, s)) {
		fprintf(f, "\t%-2u [ label = <", indexof(fsm, s));

		printcolours(fsm, s->cl, f);

		fprintf(f, "> ];\n");
	}

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		for (e = s->edges[i].sl; e != NULL; e = e->next) {
			assert(e->state != NULL);

			/*
			 * The consolidate_edges option is an aesthetic optimisation.
			 * For a state which has multiple edges all transitioning to the same
			 * state, all these edges are combined into a single edge, labelled
			 * with a more concise form of all their literal values.
			 *
			 * To implement this, we loop through all unique states, rather than
			 * looping through each edge.
			 */
			if (options->consolidate_edges) {
				static const struct bm bm_empty;
				struct bm bm;
				struct state_set *e2;
				int hi, lo;
				int k;
				int single;

				/* unique states only */
				if (contains(s->edges, i + 1, e->state)) {
					continue;
				}

				bm = bm_empty;

				/* find all edges which go to this state */
				for (k = 0; k <= FSM_EDGE_MAX; k++) {
					for (e2 = s->edges[k].sl; e2 != NULL; e2 = e2->next) {
						if (e2->state == e->state) {
							/*
							 * This confused me for a while. Duplicate edges in
							 * NFA such as [aabc] are actually only stored once
							 * per transition (i.e. [abc], because the sets in
							 * set.c do not have duplicate elements.
							 */
							assert(!bm_get(&bm, k));

							bm_set(&bm, k);
						}
					}
				}

				single = bm_single(&bm);

				fprintf(f, "\t%-2u -> %-2u [ label = <%s",
					indexof(fsm, s), indexof(fsm, e->state),
					single ? "" : "[");

				/* now print the edges we found */
				hi = -1;
				for (;;) {
					lo = bm_next(&bm, hi, 1);	/* start of range */
					if (lo > UCHAR_MAX) {
						break;
					}

					hi = bm_next(&bm, lo, 0);	/* end of range */
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

				fprintf(f, "%s> ];\n", single ? "" : "]");
			} else {
				fprintf(f, "\t%-2u -> %-2u [ label = <",
					indexof(fsm, s), indexof(fsm, e->state));

				escputc(i, f);

				fprintf(f, "> ];\n");
			}
		}
	}
}

static void out_dotfrag(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options) {
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		singlestate(fsm, f, s, options);

		if (fsm_isend(fsm, s)) {
			fprintf(f, "\t%-2u [ shape = doublecircle ];\n", indexof(fsm, s));
		}
	}
}


void out_dot(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options) {
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
	fprintf(f, "\tstart -> %u;\n", start != NULL ? indexof(fsm, start) : 0U);

	fprintf(f, "\n");

	out_dotfrag(fsm, f, options);

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

