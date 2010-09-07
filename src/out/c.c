/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "out/out.h"
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
	int i;

	assert(state != NULL);

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[0] != state->edges[i]) {
			return NULL;
		}
	}

	return state->edges[0];
}

static void singlecase(FILE *f, const struct fsm_state *state) {
	const struct fsm_state *to;
	int i;

	assert(f != NULL);
	assert(state != NULL);

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (state->edges[i] != NULL) {
			break;
		}
	}

	/* no edges */
	if (i > FSM_EDGE_MAX) {
		fprintf(f, "\t\t\treturn 0;\n");
		return;
	}

	fprintf(f, "\t\t\tswitch (c) {\n");

	/* "any" edge */
	to = findany(state);

	/* usual case */
	if (to == NULL) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			if (state->edges[i] == NULL) {
				continue;
			}

			/* TODO: pass S%u out to maximum state width */
			fprintf(f, "\t\t\tcase '");
			escputc(i, f);
			fprintf(f, "': state = S%u; continue;\n", state->edges[i]->id);
		}
	}

	if (to != NULL) {
		fprintf(f, "\t\t\tdefault:  state = S%u; continue;\n", to->id);
	} else {
		fprintf(f, "\t\t\tdefault:  return 0;\n");	/* invalid edge */
	}

	fprintf(f, "\t\t\t}\n");
}

static void stateenum(FILE *f, struct state_list *sl) {
	struct state_list *s;
	int i;

	fprintf(f, "\tenum {\n");
	fprintf(f, "\t\t");

	for (s = sl, i = 1; s; s = s->next, i++) {
		fprintf(f, "S%u", s->state.id);
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

static void endstates(FILE *f, const struct fsm *fsm, struct state_list *sl) {
	struct state_list *s;

	/* no end states */
	{
		int endcount;

		endcount = 0;
		for (s = sl; s; s = s->next) {
			endcount += !!fsm_isend(fsm, &s->state);
		}

		if (endcount == 0) {
			printf("\treturn EOF; /* unexpected EOF */\n");
			return;
		}
	}

	/* usual case */
	fprintf(f, "\t/* end states */\n");
	fprintf(f, "\tswitch (state) {\n");
	for (s = sl; s; s = s->next) {
		if (!fsm_isend(fsm, &s->state)) {
			continue;
		}

		assert(s->state.end > 0);
		fprintf(f, "\tcase S%d: return %u;\n",
			s->state.id, s->state.id);
	}
	fprintf(f, "\tdefault: return EOF; /* unexpected EOF */\n");
	fprintf(f, "\t}\n");
}

void out_c(const struct fsm *fsm, FILE *f) {
	struct state_list *s;

	assert(fsm != NULL);
	assert(fsm_isdfa(fsm));

	/* TODO: prerequisite that the FSM is a DFA */

	/* TODO: prerequisite that edges are single characters */

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
	for (s = fsm->sl; s; s = s->next) {
		fprintf(f, "\t\tcase S%u:", s->state.id);

		if (s->state.opaque != NULL) {
			fprintf(f, " /* ");
			fputs(s->state.opaque, f);
			fprintf(f, " */");
		}

		fprintf(f, "\n");
		singlecase(f, &s->state);

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

