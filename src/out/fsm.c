/* $Id$ */

#include <assert.h>
#include <stdio.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"

void out_fsm(const struct fsm *fsm, FILE *f) {
	struct state_list *s;
	struct fsm_edge *e;
	struct fsm_state *start;
	int end;

	for (s = fsm->sl; s; s = s->next) {
		for (e = s->state.edges; e; e = e->next) {
			fprintf(f, "%-2u -> %-2u", s->state.id, e->state->id);

			if (e->label->label != NULL) {
				fprintf(f, " \"%s\"", e->label->label);
			}

			fprintf(f, ";\n");
		}
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

