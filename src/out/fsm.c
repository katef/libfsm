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

void out_fsm(const struct fsm *fsm, FILE *f) {
	struct state_list *s;
	struct fsm_state *start;
	int end;

	for (s = fsm->sl; s; s = s->next) {
		int i;

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			const struct fsm_edge *e;

			e = &s->state.edges[i];

			if (e->trans == NULL) {
				continue;
			}

			assert(e->state != NULL);

			fprintf(f, "%-2u -> %-2u", s->state.id, e->state->id);

			/* TODO: print " ?" if all edges are equal */

			switch (e->trans->type) {
			case FSM_EDGE_EPSILON:
				break;

			case FSM_EDGE_LITERAL:
				fprintf(f, " \'");
				escputc(i, f);
				fprintf(f, "\'");
				break;
			}

			fprintf(f, ";\n");
		}
	}

	for (s = fsm->sl; s; s = s->next) {
		if (s->state.opaque == NULL) {
			continue;
		}

		fprintf(f, "%-2u = \"", s->state.id);
		escputs(s->state.opaque, f);
		fprintf(f, "\";\n");
	}

	fprintf(f, "\n");

	start = fsm_getstart(fsm);
	assert(start != NULL);
	fprintf(f, "start: %u;\n", start->id);

	end = 0;
	for (s = fsm->sl; s; s = s->next) {
		end += !!fsm_isend(fsm, &s->state);
	}

	if (end == 0) {
		return;
	}

	fprintf(f, "end:   ");
	for (s = fsm->sl; s; s = s->next) {
		if (fsm_isend(fsm, &s->state)) {
			end--;
			fprintf(f, "%u%s", s->state.id, end > 0 ? ", " : ";\n");
		}
	}
}

