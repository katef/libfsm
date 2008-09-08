/* $Id$ */

#include <stdio.h>

#include "fsm.h"

void out_dot(FILE *f, struct fsm_state *start, struct state_list *l) {
	struct state_list *s;
	struct fsm_edge *e;

	printf("digraph G {\n");
	printf("\trankdir = LR;\n");

	printf("\tnode [ shape = circle ];\n");

	printf("\n");

	printf("\tstart [ shape = none, label = \"\" ];\n");
	printf("\tstart -> %u;\n", start->id);

	printf("\n");

	for (s = l; s; s = s->next) {
		for (e = s->state.edges; e; e = e->next) {
			fprintf(f, "\t%-2u -> %-2u [ label = \"%s\" ];\n",
				s->state.id, e->state->id, e->label);
		}
	}

	printf("\n");

	for (s = l; s; s = s->next) {
		if (s->state.end) {
			fprintf(f, "\t%-2u [ shape = doublecircle ];\n", s->state.id);
		}
	}

	printf("};\n");
}

