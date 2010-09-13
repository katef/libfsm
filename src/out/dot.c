/* $Id$ */

#include <assert.h>
#include <stdio.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"
#include "libfsm/set.h"

static void escputc(int c, FILE *f) {
	assert(f != NULL);

	switch (c) {
	case FSM_EDGE_EPSILON:
		fprintf(f, "&epsilon;");
		return;

	case '\"':
		fprintf(f, "\\\"");
		return;

	case '|':
		fprintf(f, "\\|");
		return;

	/* TODO: others */

	default:
		putc(c, f);
	}
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
static int contains(struct state_set *edges[], int o, struct fsm_state *state) {
	int i;

	assert(edges != NULL);
	assert(state != NULL);

	for (i = o; i <= FSM_EDGE_MAX; i++) {
		if (set_contains(state, edges[i])) {
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

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		for (e = s->edges[i]; e; e = e->next) {
			assert(e->state != NULL);

			/* TODO: print "?" if all edges are equal */

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
				int k;
				struct state_set *e2;

				/* unique states only */
				if (contains(s->edges, i + 1, e->state)) {
					continue;
				}

				/* find all edges which go to this state */
				fprintf(f, "\t%-2u -> %-2u [ label = \"", s->id, e->state->id);
				for (k = 0; k <= FSM_EDGE_MAX; k++) {
					for (e2 = s->edges[k]; e2; e2 = e2->next) {
						if (e2->state != e->state) {
							continue;
						}

						escputc(k, f);

						if (set_contains(e2->state, e2->next) || contains(s->edges, k + 1, e2->state)) {
							fprintf(f, "|");
						}
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

	start = fsm_getstart(fsm);
	assert(start != NULL);
	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");
	fprintf(f, "\tstart -> %u;\n", start->id);

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

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

