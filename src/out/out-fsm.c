/* $Id$ */

#include <stdio.h>

#include <fsm/fsm.h>

#include "out/out.h"

void out_fsm(FILE *f, const struct fsm_options *options,
	struct state_list *sl, struct label_list *ll, struct fsm_state *start) {
	struct state_list *s;
	struct fsm_edge *e;
	int end;

	for (s = sl; s; s = s->next) {
		for (e = s->state.edges; e; e = e->next) {
			fprintf(f, "%-2u -> %-2u", s->state.id, e->state->id);

			if (e->label->label != NULL) {
				fprintf(f, " \"%s\"", e->label->label);
			}

			fprintf(f, ";\n");
		}
	}

	fprintf(f, "\n");

	fprintf(f, "start: %u;\n", start->id);

	end = 0;
	for (s = sl; s; s = s->next) {
		if (s->state.end) {
			end++;
		}
	}

	if (end == 0) {
		return;
	}

	fprintf(f, "end:   ");
	for (s = sl; s; s = s->next) {
		if (s->state.end) {
			end--;
			fprintf(f, "%u%s", s->state.id, end > 0 ? ", " : ";\n");
		}
	}
}

