/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"

static int escputc(char c, FILE *f) {
	assert(f != NULL);

	switch (c) {
	case '\"':
			fprintf(f, "\\\"");
			return 2;

	/* TODO: others */

	default:
			putc(c, f);
			return 1;
	}
}

static int escputs(const char *s, FILE *f) {
	int n;

	assert(f != NULL);
	assert(s != NULL);

	for (n = 0; *s; s++) {
		n += escputc(*s, f);
	}

	return n;
}

static void hr(FILE *f, struct state_list *sl) {
	struct state_list *x;

	assert(f != NULL);
	assert(sl != NULL);

	fprintf(f, "----------");
	for (x = sl; x; x = x->next) {
		fprintf(f, "+----");
	}

	fprintf(f, "\n");
}

void out_table(const struct fsm *fsm, FILE *f) {
	struct state_list *x;
	struct trans_list *t;
	struct fsm_edge   *e;

	/* TODO: assert! */

	fprintf(f, "%-9s ", "");
	for (x = fsm->sl; x; x = x->next) {
		fprintf(f, "| %-2u ", x->state.id);
	}

	fprintf(f, "\n");

	hr(f, fsm->sl);

	for (t = fsm->tl; t; t = t->next) {
		{
			int n = 0;

			switch (t->type) {
			case FSM_EDGE_EPSILON:
				n = fprintf(f, "epsilon");
				break;

			case FSM_EDGE_LITERAL:
				n = escputc(t->u.literal, f);
				break;

			case FSM_EDGE_LABEL:
				n = escputs(t->u.label, f);
				break;
			}

			if (n > 9) {
				n = 9;
			}

			assert(n >= 0);
			fprintf(f, "%.*s ", 9 - n, "         ");
		}

		for (x = fsm->sl; x; x = x->next) {
			for (e = x->state.edges; e; e = e->next) {
				if (t == e->trans) {
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

