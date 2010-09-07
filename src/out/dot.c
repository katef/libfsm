/* $Id$ */

#include <assert.h>
#include <stdio.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"

static void escputc(char c, FILE *f) {
	assert(f != NULL);

	switch (c) {
	case '\"':
		fprintf(f, "\\\"");
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

void out_dot(const struct fsm *fsm, FILE *f) {
	struct state_list *s;
	struct epsilon_list *e;
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
		int i;

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			const struct fsm_edge *e;

			e = &s->state.edges[i];

			if (e->state == NULL) {
				continue;
			}

			assert(e->state);

			/* TODO: print "?" if all edges are equal */

			fprintf(f, "\t%-2u -> %-2u [ label = \"", s->state.id, e->state->id);
			escputc(i, f);
			fprintf(f, "\" ];\n");
		}

		for (e = s->state.el; e; e = e->next) {
			fprintf(f, "\t%-2u -> %-2u [ label = \"&epsilon;\" ];\n",
				s->state.id, e->state->id);
		}
	}

	for (s = fsm->sl; s; s = s->next) {
		if (s->state.opaque == NULL) {
			continue;
		}

		fprintf(f, "\t%-2u [ color = \"", s->state.id);
		escputs(s->state.opaque, f);
		fprintf(f, "\" ];\n");
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

