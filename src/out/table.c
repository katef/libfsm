/* $Id$ */

#include <stdio.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"

static void hr(FILE *f, struct state_list *sl) {
	struct state_list *x;

	fprintf(f, "----------");
	for (x = sl; x; x = x->next) {
		fprintf(f, "+----");
	}

	fprintf(f, "\n");
}

void out_table(const struct fsm *fsm, FILE *f,
	int (*put)(const char *s, FILE *f)) {
	struct state_list *x;
	struct label_list *y;
	struct fsm_edge   *e;

	/* TODO: assert! */

	if (!put) {
		/* TODO: default to escaping special characters? (maybe just non-printable ones) */
		put = fputs;
	}

	fprintf(f, "%-9s ", "");
	for (x = fsm->sl; x; x = x->next) {
		fprintf(f, "| %-2u ", x->state.id);
	}

	fprintf(f, "\n");

	hr(f, fsm->sl);

	for (y = fsm->ll; y; y = y->next) {
		const char *label;

		label = y->label == NULL ? "epsilon" : y->label;
		put(label, f);
		fprintf(f, "%.*s ", 9 - (int) strlen(label), "         ");

		for (x = fsm->sl; x; x = x->next) {
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

	hr(f, fsm->sl);

	fprintf(f, "%-9s ", "start/end");

	for (x = fsm->sl; x; x = x->next) {
		fprintf(f, "| %s%s ",
			&x->state == fsm_getstart(fsm) ? "S" : " ",
			fsm_isend(fsm, &x->state) ? "E" : " ");
	}

	fprintf(f, "\n");
}

