/* $Id$ */

#include <assert.h>
#include <stdio.h>

#include <fsm/fsm.h>

#include "out.h"
#include "libfsm/internal.h"
#include "libfsm/set.h"
#include "libfsm/colour.h"

static unsigned int
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	unsigned int i;

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

static void escputc(char c, FILE *f) {
	assert(f != NULL);

	switch (c) {
	case '\'':
		fprintf(f, "\"'\"");
		return;

	case '\"':
		fprintf(f, "'\"'");
		return;

	/* TODO: others */

	default:
		putc('\'', f);
		putc(c, f);
		putc('\'', f);
	}
}

/* TODO: centralise */
static void escputs(const char *s, FILE *f) {
	const char *p;

	assert(f != NULL);
	assert(s != NULL);

	for (p = s; *p != '\0'; p++) {
		escputc(*p, f);
	}
}

/* TODO: centralise */
static const struct fsm_state *findany(const struct fsm_state *state) {
	struct state_set *e;
	int i;

	assert(state != NULL);

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i].sl == NULL) {
			return NULL;
		}

		for (e = state->edges[i].sl; e != NULL; e = e->next) {
			if (e->state != state->edges[0].sl->state) {
				return NULL;
			}
		}
	}

	assert(state->edges[0].sl != NULL);

	return state->edges[0].sl->state;
}

void out_fsm(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options) {
	struct fsm_state *s;
	struct state_set *e;
	struct colour_set *c;
	struct fsm_state *start;
	int end;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		int i;

		{
			const struct fsm_state *a;

			a = findany(s);
			if (a != NULL) {
				fprintf(f, "%-2u -> %2u ?;\n", indexof(fsm, s), indexof(fsm, a));
				continue;
			}
		}

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (e = s->edges[i].sl; e != NULL; e = e->next) {
				assert(e->state != NULL);

				fprintf(f, "%-2u -> %2u", indexof(fsm, s), indexof(fsm, e->state));

				/* TODO: print " ?" if all edges are equal */

				switch (i) {
				case FSM_EDGE_EPSILON:
					break;

				default:
					putc(' ', f);
					escputc(i, f);
					break;
				}

				fprintf(f, ";\n");
			}

			if (fsm->colour_hooks.print != NULL) {
				for (c = s->edges[i].cl; c != NULL; c = c->next) {
					fprintf(f, "%-2u ", indexof(fsm, s));
					escputc(i, f);
					fprintf(f, " = \"");

					/* TODO: escapes */
					fsm->colour_hooks.print(fsm, f, c->colour);

					fprintf(f, "\";\n");
				}
			}
		}
	}

	fprintf(f, "\n");

	start = fsm_getstart(fsm);
	if (start == NULL) {
		return;
	}

	fprintf(f, "start: %u;\n", indexof(fsm, start));

	end = 0;
	for (s = fsm->sl; s != NULL; s = s->next) {
		end += !!fsm_isend(fsm, s);
	}

	if (end == 0) {
		return;
	}

	fprintf(f, "end:   ");
	for (s = fsm->sl; s != NULL; s = s->next) {
		if (fsm_isend(fsm, s)) {
			struct colour_set *c;

			end--;

			if (s->cl == NULL || fsm->colour_hooks.print == NULL) {
				fprintf(f, "%u", indexof(fsm, s));
			} else for (c = s->cl; c != NULL; c = c->next) {
				fprintf(f, "%u = \"", indexof(fsm, s));

				/* TODO: escapes */
				fsm->colour_hooks.print(fsm, f, c->colour);

				fprintf(f, "\"");

				if (c->next != NULL) {
					fprintf(f, ", ");
				}
			}

			fprintf(f, "%s", end > 0 ? ", " : ";\n");
		}
	}
}

