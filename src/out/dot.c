/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"
#include "libfsm/set.h"

struct bm {
	unsigned char map[(FSM_EDGE_MAX + 1) / CHAR_BIT];
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

static void escputc(int c, FILE *f) {
	assert(f != NULL);

	switch (c) {
	case FSM_EDGE_EPSILON:
		fprintf(f, "&epsilon;");
		return;

	case '\\':
		fprintf(f, "\\\\");
		return;

	case '\n':
		fprintf(f, "\\n");
		return;

	case '\r':
		fprintf(f, "\\r");
		return;

	case '\f':
		fprintf(f, "\\f");
		return;

	case '\v':
		fprintf(f, "\\v");
		return;

	case '\t':
		fprintf(f, "\\t");
		return;

	case ' ':
		fprintf(f, "&lsquo; &rsquo;");
		return;

	case '\"':
		fprintf(f, "&lsquo;\\\"&rsquo;");
		return;

	case '\'':
		fprintf(f, "&lsquo;\\'&rsquo;");
		return;

	case '.':
		fprintf(f, "&lsquo;.&rsquo;");
		return;

	case '|':
		fprintf(f, "&lsquo;|&rsquo;");
		return;

	/* TODO: others */
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

	for (p = s; *p; p++) {
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

static void singlestate(const struct fsm *fsm, FILE *f, struct fsm_state *s) {
	struct state_set *e;
	int i;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(s != NULL);

	/* TODO: findany() here? */

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		for (e = s->edges[i].sl; e; e = e->next) {
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
			if (fsm->options.consolidate_edges) {
				static const struct bm bm_empty;
				struct bm bm;
				struct state_set *e2;
				int hi, lo;
				int k;

				/* unique states only */
				if (contains(s->edges, i + 1, e->state)) {
					continue;
				}

				bm = bm_empty;

				/* find all edges which go to this state */
				fprintf(f, "\t%-2u -> %-2u [ label = \"", s->id, e->state->id);
				for (k = 0; k <= FSM_EDGE_MAX; k++) {
					for (e2 = s->edges[k].sl; e2; e2 = e2->next) {
						if (e2->state == e->state) {
							bm_set(&bm, k);
						}
					}
				}

				/* now print the edges we found */
				hi = -1;
				for (;;) {
					lo = bm_next(&bm, hi, 1);	/* start of range */
					if (lo == FSM_EDGE_MAX + 1) {
						break;
					}

					if (hi != -1) {
						fputc('|', f);
					}

					hi = bm_next(&bm, lo, 0);	/* end of range */

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
						fprintf(f, "..");
						escputc(hi - 1, f);
						break;
					}
				}

				fprintf(f, "\" ];\n");
			} else {
				fprintf(f, "\t%-2u -> %-2u [ label = \"", s->id, e->state->id);
				escputc(i, f);
				fprintf(f, "\" ];\n");
			}
		}
	}
}

void out_dot(const struct fsm *fsm, FILE *f) {
	struct fsm_state *s;
	struct fsm_state *start;

	/* TODO: assert! */

	fprintf(f, "digraph G {\n");
	fprintf(f, "\trankdir = LR;\n");

	fprintf(f, "\tnode [ shape = circle ];\n");

	if (fsm->options.anonymous_states) {
		fprintf(f, "\tnode [ label = \"\", width = 0.3 ];\n");
	}

	fprintf(f, "\n");

	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");

	start = fsm_getstart(fsm);
	fprintf(f, "\tstart -> %u;\n", start != NULL ? start->id : 0U);

	fprintf(f, "\n");

	for (s = fsm->sl; s; s = s->next) {
		struct opaque_set *o;

		singlestate(fsm, f, s);

		for (o = s->ol; o; o = o->next) {
			fprintf(f, "\t%-2u [ color = \"", s->id);
			escputs(o->opaque, f);
			fprintf(f, "\" ];\n");
		}

		if (fsm_isend(fsm, s)) {
			fprintf(f, "\t%-2u [ shape = doublecircle ];\n", s->id);
		}
	}

	if (start == NULL) {
		fprintf(f, "\t0 [ shape = doublecircle ];\n");
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

