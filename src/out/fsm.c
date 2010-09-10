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

void out_fsm(const struct fsm *fsm, FILE *f) {
	struct fsm_state *s;
	struct state_set *e;
	struct fsm_state *start;
	int end;

	for (s = fsm->sl; s; s = s->next) {
		int i;

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			if (s->edges[i] == NULL) {
				continue;
			}

			fprintf(f, "%-2u -> %2u", s->id, s->edges[i]->id);

			/* TODO: print " ?" if all edges are equal */

			fprintf(f, " \'");
			escputc(i, f);
			fprintf(f, "\';\n");
		}

		for (e = s->el; e != NULL; e = e->next) {
			fprintf(f, "%-2u -> %2u;\n", s->id, e->state->id);
		}
	}

	for (s = fsm->sl; s; s = s->next) {
		struct opaque_set *o;

		for (o = s->ol; o; o = o->next) {
			fprintf(f, "%-2u = \"", s->id);
			escputs(o->opaque, f);
			fprintf(f, "\";\n");
		}
	}

	fprintf(f, "\n");

	start = fsm_getstart(fsm);
	assert(start != NULL);
	fprintf(f, "start: %u;\n", start->id);

	end = 0;
	for (s = fsm->sl; s; s = s->next) {
		end += !!fsm_isend(fsm, s);
	}

	if (end == 0) {
		return;
	}

	fprintf(f, "end:   ");
	for (s = fsm->sl; s; s = s->next) {
		if (fsm_isend(fsm, s)) {
			end--;
			fprintf(f, "%u%s", s->id, end > 0 ? ", " : ";\n");
		}
	}
}

