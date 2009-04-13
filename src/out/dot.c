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
	struct fsm_edge *e;
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
		for (e = s->state.edges; e; e = e->next) {
			assert(e->state);
			assert(e->trans);

			fprintf(f, "\t%-2u -> %-2u [ label = \"", s->state.id, e->state->id);

			switch (e->trans->type) {
			case FSM_EDGE_EPSILON:
				fputs("&epsilon;", f);
				break;

			case FSM_EDGE_ANY:
				fputs("?", f);
				break;

			case FSM_EDGE_LITERAL:
				escputc(e->trans->u.literal, f);
				break;

			case FSM_EDGE_LABEL:
				escputs(e->trans->u.label, f);
				break;
			}

			fprintf(f, "\" ];\n");
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

