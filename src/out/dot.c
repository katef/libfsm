/* $Id$ */

#include <assert.h>
#include <stdio.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"

void out_dot(const struct fsm *fsm, FILE *f) {
	struct state_list *s;
	struct fsm_edge *e;
	struct fsm_state *start;

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
		for (e = s->state.edges; e; e = e->next) {
			const char *label;

			label = e->label->label == NULL ? "&epsilon;" : e->label->label;
			fprintf(f, "\t%-2u -> %-2u [ label = \"%s\" ];\n",
				s->state.id, e->state->id, label);
		}
	}

	fprintf(f, "\n");

	for (s = fsm->sl; s; s = s->next) {
		if (fsm_isend(fsm, &s->state)) {
			fprintf(f, "\t%-2u [ shape = doublecircle ];\n", s->state.id);
		}
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

