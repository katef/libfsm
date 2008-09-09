/* $Id$ */

#include <stdio.h>

#include "fsm.h"
#include "out.h"

void out_dot(FILE *f, const struct fsm_options *options,
	struct state_list *sl, struct label_list *ll, struct fsm_state *start) {
	struct state_list *s;
	struct fsm_edge *e;

	fprintf(f, "digraph G {\n");
	fprintf(f, "\trankdir = LR;\n");

	fprintf(f, "\tnode [ shape = circle ];\n");

	if (options->anonymous_states) {
		fprintf(f, "\tnode [ label = \"\", width = 0.3 ];\n");
	}

	fprintf(f, "\n");

	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");
	fprintf(f, "\tstart -> %u;\n", start->id);

	fprintf(f, "\n");

	for (s = sl; s; s = s->next) {
		for (e = s->state.edges; e; e = e->next) {
			fprintf(f, "\t%-2u -> %-2u [ label = \"%s\" ];\n",
				s->state.id, e->state->id, e->label->label);
		}
	}

	fprintf(f, "\n");

	for (s = sl; s; s = s->next) {
		if (s->state.end) {
			fprintf(f, "\t%-2u [ shape = doublecircle ];\n", s->state.id);
		}
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

