/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <fsm/fsm.h>

#include "out.h"
#include "libfsm/set.h"
#include "libfsm/internal.h"

static unsigned
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 0; s != NULL; s = s->next, i++) {
		if (s == state) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

static int escputc(int c, FILE *f) {
	assert(f != NULL);

	switch (c) {
	case FSM_EDGE_EPSILON:
		fprintf(f, "epsilon");
		return 7;

	case '\"':
		fprintf(f, "\\\"");
		return 2;

	/* TODO: others */

	default:
		if (!isprint(c)) {
			return printf("0x%02X", (unsigned char) c);
		}

		putc(c, f);
		return 1;
	}
}

static void hr(FILE *f, struct fsm_state *sl) {
	struct fsm_state *s;

	assert(f != NULL);

	fprintf(f, "----------");
	for (s = sl; s != NULL; s = s->next) {
		fprintf(f, "+----");
	}

	fprintf(f, "\n");
}

static int notransitions(struct fsm_state *sl, int i) {
	struct fsm_state *s;

	assert(i >= 0);
	assert(i <= FSM_EDGE_MAX);

	for (s = sl; s != NULL; s = s->next) {
		if (s->edges[i].sl != NULL) {
			return 0;
		}
	}

	return 1;
}

void out_table(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options) {
	struct fsm_state *s;
	int i;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	fprintf(f, "%-9s ", "");
	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "| %-2u ", indexof(fsm, s));
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

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (s->edges[i].sl == NULL) {
				fprintf(f, "|    ");
			} else {
				assert(s->edges[i].sl->state != NULL);

				fprintf(f, "| %-2u ", indexof(fsm, s->edges[i].sl->state));
			}
		}

		fprintf(f, "\n");
	}

	hr(f, fsm->sl);

	fprintf(f, "%-9s ", "start/end");

	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "| %s%s ",
			s == fsm_getstart(fsm) ? "S" : " ",
			fsm_isend(fsm, s)      ? "E" : " ");
	}

	fprintf(f, "\n");
}

