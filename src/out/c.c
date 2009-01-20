/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <fsm/fsm.h>

#include "out/out.h"
#include "libfsm/internal.h"

static void singlecase(FILE *f, const struct fsm_state *state) {
	struct fsm_edge *e;

	/* no edges */
	if (state->edges == NULL) {
		fprintf(f, "\t\t\treturn 0;\n");
		return;
	}

	/* usual case */
	fprintf(f, "\t\t\tswitch (c) {\n");
	for (e = state->edges; e; e = e->next) {
		assert(e->label->label != NULL);
		assert(strlen(e->label->label) == 1);

		fprintf(f, "\t\t\tcase '");
		/* TODO: escape special characters C-style. one character only */
		fputs(e->label->label, f);
		fprintf(f, "': state = S%u; continue;\n", e->state->id);
	}

	fprintf(f, "\t\t\tdefault:  return 0;\n");	/* invalid edge */
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

		assert(s->state.end <= INT_MAX && s->state.end > 0);
		fprintf(f, "\tcase S%d: return %u;\n",
			s->state.id, s->state.end);
	}
	fprintf(f, "\tdefault: return EOF; /* unexpected EOF */\n");
	fprintf(f, "\t}\n");
}

void out_c(const struct fsm *fsm, FILE *f) {
	struct state_list *s;

	assert(fsm != NULL);

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
		fprintf(f, "\t\tcase S%u:\n", s->state.id);
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

