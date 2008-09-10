/* $Id$ */

#include <stdio.h>

#include <fsm/fsm.h>

#include "out/out.h"

static void hr(FILE *f, struct state_list *sl) {
	struct state_list *x;

	fprintf(f, "---------");
	for (x = sl; x; x = x->next) {
		fprintf(f, "+----");
	}

	fprintf(f, "\n");
}

void out_table(FILE *f, const struct fsm_options *options,
	struct state_list *sl, struct label_list *ll, struct fsm_state *start) {
	struct state_list *x;
	struct label_list *y;
	struct fsm_edge   *e;

	(void) options;
	(void) start;

	fprintf(f, "%-8s ", "");
	for (x = sl; x; x = x->next) {
		fprintf(f, "| %-2u ", x->state.id);
	}

	fprintf(f, "\n");

	hr(f, sl);

	for (y = ll; y; y = y->next) {
		const char *label;

		label = y->label == NULL ? "epsilon" : y->label;
		fprintf(f, "%-8s ", label);

		for (x = sl; x; x = x->next) {
			for (e = x->state.edges; e; e = e->next) {
				if (y == e->label) {
					fprintf(f, "| %-2u ", e->state->id);
					break;
				}
			}

			if (e == NULL) {
				fprintf(f, "|    ");
			}
		}

		fprintf(f, "\n");
	}

	hr(f, sl);

	fprintf(f, "%-8s ", "end");

	/* TODO: indicate start state, too */
	for (x = sl; x; x = x->next) {
		fprintf(f, "| %s  ", x->state.end ? "E" : " ");
	}

	fprintf(f, "\n");
}

