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
	struct fsm_state *s;
	struct state_set *e;
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
			if (s->edges[i] == NULL) {
				continue;
			}

			/* TODO: print "?" if all edges are equal */

			fprintf(f, "\t%-2u -> %-2u [ label = \"", s->id, s->edges[i]->id);
			escputc(i, f);
			fprintf(f, "\" ];\n");
		}

		for (e = s->el; e; e = e->next) {
			fprintf(f, "\t%-2u -> %-2u [ label = \"&epsilon;\" ];\n",
				s->id, e->state->id);
		}
	}

	for (s = fsm->sl; s; s = s->next) {
		struct opaque_set *o;

		for (o = s->ol; o; o = o->next) {
			fprintf(f, "\t%-2u [ color = \"", s->id);
			escputs(o->opaque, f);
			fprintf(f, "\" ];\n");
		}
	}

	fprintf(f, "\n");

	for (s = fsm->sl; s; s = s->next) {
		if (fsm_isend(fsm, s)) {
			fprintf(f, "\t%-2u [ shape = doublecircle ];\n", s->id);
		}
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

