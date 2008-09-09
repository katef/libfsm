/* $Id$ */

#include <stdio.h>

#include "fsm.h"

void out_dot(FILE *f, struct state_list *l, struct fsm_state *start) {
	struct state_list *s;
	struct fsm_edge *e;

	fprintf(f, "digraph G {\n");
	fprintf(f, "\trankdir = LR;\n");

	fprintf(f, "\tnode [ shape = circle ];\n");

	fprintf(f, "\n");

	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");
	fprintf(f, "\tstart -> %u;\n", start->id);

	fprintf(f, "\n");

	for (s = l; s; s = s->next) {
		for (e = s->state.edges; e; e = e->next) {
			fprintf(f, "\t%-2u -> %-2u [ label = \"%s\" ];\n",
				s->state.id, e->state->id, e->label);
		}
	}

	fprintf(f, "\n");

	for (s = l; s; s = s->next) {
		if (s->state.end) {
			fprintf(f, "\t%-2u [ shape = doublecircle ];\n", s->state.id);
		}
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

