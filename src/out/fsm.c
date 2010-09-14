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

static void escputs(const char *s, FILE *f) {
	const char *p;

	assert(f != NULL);
	assert(s != NULL);

	for (p = s; *p; p++) {
		escputc(*p, f);
	}
}

/* TODO: refactor for when FSM_EDGE_ANY goes; it is an "any" transition if all
 * labels transition to the same state. centralise that, perhaps */
/* TODO: centralise */
static const struct fsm_state *findany(const struct fsm_state *state) {
	struct state_set *e;
	int i;

	assert(state != NULL);

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i] == NULL) {
			return NULL;
		}

		for (e = state->edges[i]; e; e = e->next) {
			if (e->state != state->edges[0]->state) {
				return NULL;
			}
		}
	}

	assert(state->edges[0] != NULL);

	return state->edges[0]->state;
}

void out_fsm(const struct fsm *fsm, FILE *f) {
	struct fsm_state *s;
	struct state_set *e;
	struct fsm_state *start;
	int end;

	for (s = fsm->sl; s; s = s->next) {
		int i;

		{
			const struct fsm_state *a;

			a = findany(s);
			if (a != NULL) {
				fprintf(f, "%-2u -> %2u ?;\n", s->id, a->id);
				continue;
			}
		}

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (e = s->edges[i]; e; e = e->next) {
				assert(e->state != NULL);

				fprintf(f, "%-2u -> %2u", s->id, e->state->id);

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

