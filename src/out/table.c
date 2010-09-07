/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"

static int escputc(char c, FILE *f) {
	assert(f != NULL);

	if (!isprint(c)) {
		return printf("0x%02X", (unsigned char) c);
	}

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

static int notransitions(struct state_list *sl, int i) {
	struct state_list *x;

	assert(i >= 0);
	assert(i <= UCHAR_MAX);

	for (x = sl; x; x = x->next) {
		if (x->state.edges[i].state != NULL) {
			return 0;
		}
	}

	return 1;
}

void out_table(const struct fsm *fsm, FILE *f) {
	struct state_list *x;
	int i;

	/* TODO: assert! */

	fprintf(f, "%-9s ", "");
	for (x = fsm->sl; x; x = x->next) {
		fprintf(f, "| %-2u ", x->state.id);
	}

	fprintf(f, "\n");

	hr(f, fsm->sl);

	/* TODO: this is sideways. now we have single-char edges only, pivot */

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		{
			int n = 0;

			/* TODO: print "?" for edges which are all equal */

			/* skip edges with no transitions */
			if (notransitions(fsm->sl, i)) {
				continue;
			}

			n = escputc(i, f);
			if (n > 9) {
				n = 9;
			}

			assert(n >= 0);
			fprintf(f, "%.*s ", 9 - n, "         ");
		}

		for (x = fsm->sl; x; x = x->next) {
			const struct fsm_edge *e;

			e = &x->state.edges[i];

			if (e->state == NULL) {
				fprintf(f, "|    ");
			} else {
				fprintf(f, "| %-2u ", e->state->id);
			}
		}

		fprintf(f, "\n");
	}

	/* TODO: only if there are epislon transitions for at least one state */
	{
		struct epsilon_list *e;

		fprintf(f, "%-9s ", "epsilon");

		for (x = fsm->sl; x; x = x->next) {
			for (e = x->state.el; e; e = e->next) {
				assert(e->state != NULL);

				if (e->state == &x->state) {
					break;
				}
			}

			if (e == NULL) {
				fprintf(f, "|    ");
				continue;
			}

			assert(e->state != NULL);

			fprintf(f, "| %-2u ", e->state->id);
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

