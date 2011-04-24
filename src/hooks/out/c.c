/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "out.h"
#include "libfsm/set.h"
#include "libfsm/colour.h"
#include "libfsm/internal.h"

static void escputc(char c, FILE *f) {
	assert(f != NULL);

	switch (c) {
	case '\"':
		fprintf(f, "\\\"");
		return;

		/* TODO: others */

	default:
		/* TODO: if (isprint(c)) { putc(c, f); } else { hex } */
		putc(c, f);
	}
}

/* TODO: refactor for when FSM_EDGE_ANY goes; it is an "any" transition if all
 * labels transition to the same state. centralise that, perhaps */
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

/* Return true if the edges after o contains state */
/* TODO: centralise */
static int contains(struct fsm_edge edges[], int o, struct fsm_state *state) {
	int i;

	assert(edges != NULL);
	assert(state != NULL);

	for (i = o; i <= UCHAR_MAX; i++) {
		if (set_contains(state, edges[i].sl)) {
			return 1;
		}
	}

	return 0;
}

static void singlecase(FILE *f, struct fsm_state *state) {
	const struct fsm_state *to;
	int i;

	assert(f != NULL);
	assert(state != NULL);

	/* TODO: move this out into a count function */
	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i].sl != NULL) {
			break;
		}
	}

	/* no edges */
	if (i > UCHAR_MAX) {
		fprintf(f, "\t\t\treturn 0;\n");
		return;
	}

	fprintf(f, "\t\t\tswitch (c) {\n");

	/* "any" edge */
	to = findany(state);

	/* usual case */
	if (to == NULL) {
		for (i = 0; i <= UCHAR_MAX; i++) {
			if (state->edges[i].sl == NULL) {
				continue;
			}

			assert(state->edges[i].sl->state != NULL);
			assert(state->edges[i].sl->next  == NULL);

			fprintf(f, "\t\t\tcase '");
			escputc(i, f);
			fprintf(f, "':");

			/* non-unique states fall through */
			if (contains(state->edges, i + 1, state->edges[i].sl->state)) {
				fprintf(f, "\n");
				continue;
			}

			/* TODO: pass S%u out to maximum state width */
			if (state->edges[i].sl->state->id != state->id) {
				fprintf(f, " state = S%u; continue;\n", state->edges[i].sl->state->id);
			} else {
				fprintf(f, "             continue;\n");
			}
		}
	}

	if (to != NULL) {
		fprintf(f, "\t\t\tdefault:  state = S%u; continue;\n", to->id);
	}

	fprintf(f, "\t\t\t}\n");
}

static void stateenum(FILE *f, struct fsm_state *sl) {
	struct fsm_state *s;
	int i;

	fprintf(f, "\tenum {\n");
	fprintf(f, "\t\t");

	for (s = sl, i = 1; s != NULL; s = s->next, i++) {
		fprintf(f, "S%u", s->id);
		if (s->next != NULL) {
			fprintf(f, ", ");
		}

		if (i % 10 == 0) {
			fprintf(f, "\n");
			fprintf(f, "\t\t");
		}
	}

	fprintf(f, "\n");
	fprintf(f, "\t} state;\n");
}

static void endstates(FILE *f, const struct fsm *fsm, struct fsm_state *sl) {
	struct fsm_state *s;

	/* no end states */
	{
		int endcount;

		endcount = 0;
		for (s = sl; s != NULL; s = s->next) {
			endcount += !!fsm_isend(fsm, s);
		}

		if (endcount == 0) {
			printf("\treturn EOF; /* unexpected EOF */\n");
			return;
		}
	}

	/* usual case */
	fprintf(f, "\t/* end states */\n");
	fprintf(f, "\tswitch (state) {\n");
	for (s = sl; s != NULL; s = s->next) {
		if (!fsm_isend(fsm, s)) {
			continue;
		}

		assert(s->end > 0);
		fprintf(f, "\tcase S%d: return %u;\n", s->id, s->id);
	}
	fprintf(f, "\tdefault: return EOF; /* unexpected EOF */\n");
	fprintf(f, "\t}\n");
}

void out_c(const struct fsm *fsm, FILE *f) {
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(fsm_isdfa(fsm));

	/* TODO: prerequisite that the FSM is a DFA */

	/* TODO: pass in %s prefix (default to "fsm_") */

	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "\n");
	fprintf(f, "extern int fsm_getc(void *opaque);\n");

	fprintf(f, "\n");

	fprintf(f, "int fsm_main(void *opaque) {\n");
	fprintf(f, "\tint c;\n");
	fprintf(f, "\n");

	/* enum of states */
	stateenum(f, fsm->sl);
	fprintf(f, "\n");

	/* start state */
	assert(fsm->start != NULL);
	fprintf(f, "\tstate = S%u;\n", fsm->start->id);
	fprintf(f, "\n");

	/* FSM */
	fprintf(f, "\twhile ((c = fsm_getc(opaque)) != EOF) {\n");
	fprintf(f, "\t\tswitch (state) {\n");
	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "\t\tcase S%u:", s->id);

		if (s->cl != NULL) {
			struct colour_set *c;

			fprintf(f, " /*");
			for (c = s->cl; c != NULL; c = c->next) {
				fprintf(f, " %s", (char *) c->colour);
			}
			fprintf(f, " */");
		}

		fprintf(f, "\n");
		singlecase(f, s);

		if (s->next != NULL) {
			fprintf(f, "\n");
		}
	}
	fprintf(f, "\t\t}\n");
	fprintf(f, "\t}\n");

	fprintf(f, "\n");

	/* end states */
	endstates(f, fsm, fsm->sl);
	fprintf(f, "}\n");
	fprintf(f, "\n");
}

