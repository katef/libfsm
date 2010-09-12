/* $Id$ */

#include <assert.h>
#include <stdio.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"
#include "libfsm/set.h"

static void escputc(char c, FILE *f) {
	assert(f != NULL);

	switch (c) {
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

/* TODO: centralise */
static unsigned edgecount(struct fsm_state *s) {
	int i;
	unsigned count;

	assert(s != NULL);

	count = 0;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		count += s->edges[i] != NULL;
	}

	return count;
}

static int duplicateedge(struct fsm_state *a, struct fsm_state *b) {
	int i;

	assert(a != NULL);
	assert(b != NULL);

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (a->edges[i] != NULL && b->edges[i] != NULL) {
			return 1;
		}
	}

	return 0;
}

/* Return true if the edges after o contains state */
static int contains(struct fsm_state *edges[], int o, struct fsm_state *state) {
	int i;

	assert(edges != NULL);
	assert(state != NULL);

	for (i = o; i <= FSM_EDGE_MAX; i++) {
		if (edges[i] == state) {
			return 1;
		}
	}

	return 0;
}

static void singlestate(const struct fsm *fsm, FILE *f, struct fsm_state *s, struct fsm_state *origin) {
	struct state_set *e;
	int i;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(s != NULL);

	if (s == origin) {
		/* TODO: infinite recursion... output an epsilon here instead? */
		return;
	}

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (s->edges[i] == NULL) {
			continue;
		}

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

			/* unique states only */
			if (contains(s->edges, i + 1, s->edges[i])) {
				continue;
			}

			/* find all edges which go to this state */
			fprintf(f, "\t%-2u -> %-2u [ label = \"", origin == NULL ? s->id : origin->id, s->edges[i]->id);
			for (k = 0; k <= FSM_EDGE_MAX; k++) {
				if (s->edges[k] != s->edges[i]) {
					continue;
				}

				escputc(k, f);

				if (contains(s->edges, k + 1, s->edges[k])) {
					fprintf(f, "|");
				}
			}
			fprintf(f, "\" ];\n");
		} else {
			fprintf(f, "\t%-2u -> %-2u [ label = \"", origin == NULL ? s->id : origin->id, s->edges[i]->id);
			escputc(i, f);
			fprintf(f, "\" ];\n");
		}
	}

	/*
	 * The traverse_epsilons option is an aesthetic optimisation. The internal
	 * form for storing an FSM (both NFA and DFA) currently uses an array
	 * per state which holds edges, indexed by the character for each edge.
	 * Since this is physically unable to express duplicate entries, for NFA
	 * which require duplicate edges, fsm_addedge_literal() internally creates
	 * an epsilon transition to a newly-created state, and uses that state to
	 * hold the duplicate edge entry.
	 *
	 * The traverse_epsilons option attempts to "fold in" these epsilon edges on
	 * the fly, which gives the effect of having several duplicate edges per
	 * per state, as the user might expect to have created.
	 *
	 * An intentionally-created epsilon witb a single edge is indistinguishable
	 * from the internally-created state asm_addedge_literal() produces, and so
	 * this optimisation is made optional, as it is possible that it could
	 * accidentally fold in desired epsilons.
	 */
	if (fsm->options.traverse_epsilons) {
		for (e = s->el; e; e = e->next) {
			if (edgecount(e->state) == 1 && e->state->el == NULL && duplicateedge(e->state, s)) {
				singlestate(fsm, f, e->state, s);
			} else {
				fprintf(f, "\t%-2u -> %-2u [ label = \"&epsilon;\" ];\n",
					s->id, e->state->id);
			}
		}
	} else {
		assert(origin == NULL);

		for (e = s->el; e; e = e->next) {
			fprintf(f, "\t%-2u -> %-2u [ label = \"&epsilon;\" ];\n",
				s->id, e->state->id);
		}
	}
}

static int reachablebyanedge(const struct fsm *fsm, struct fsm_state *state) {
	struct fsm_state *s;
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	if (state == fsm->start) {
		return 1;
	}

	for (s = fsm->sl; s; s = s->next) {
		if (s == state) {
			continue;
		}

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			if (s->edges[i] == state) {
				return 1;
			}
		}
	}

	return 0;
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

		if (fsm->options.traverse_epsilons) {
			/*
			 * These should only be items which were dealt with during folding
			 * in of epsilons, hence redundant, and are skipped.
			 *
			 * TODO: I am not convinced that this is correct in all cases.
			 */
			if (!reachablebyanedge(fsm, s)) {
				continue;
			}
		}

		singlestate(fsm, f, s, NULL);

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

