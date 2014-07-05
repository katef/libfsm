/* $Id$ */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "out.h"
#include "libfsm/set.h"
#include "libfsm/internal.h"

static unsigned int
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	unsigned int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 1; s != NULL; s = s->next, i++) {
		if (s == state) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

static void
escputc(int c, FILE *f)
{
	size_t i;

	struct {
		int in;
		const char *out;
	} esc[] = {
		{ '\"', "\\\"" },
		{ '\\', "\\\\" },
		{ '\n', "\\n"  },
		{ '\r', "\\r"  },
		{ '\f', "\\f"  },
		{ '\v', "\\v"  },
		{ '\t', "\\t"  }
	};

	assert(c != FSM_EDGE_EPSILON);
	assert(f != NULL);

	if (!isprint(c)) {
		fprintf(f, "\\x%x", (unsigned char) c);
		return;
	}

	for (i = 0; i < sizeof esc / sizeof *esc; i++) {
		if (esc[i].in == c) {
			fputs(esc[i].out, f);
			return;
		}
	}

	putc(c, f);
}

/* Return true if the edges after o contains state */
/* TODO: centralise */
static int
contains(struct fsm_edge edges[], int o, struct fsm_state *state)
{
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

static void
singlecase(FILE *f, const struct fsm *fsm, struct fsm_state *state)
{
	int i;

	assert(fsm != NULL);
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

	/* usual case */
	{
		struct fsm_state *mode;

		mode = fsm_iscompletestate(fsm, state) ? fsm_findmode(state) : NULL;

		for (i = 0; i <= UCHAR_MAX; i++) {
			if (state->edges[i].sl == NULL) {
				continue;
			}

			assert(state->edges[i].sl->state != NULL);
			assert(state->edges[i].sl->next  == NULL);

			if (state->edges[i].sl->state == mode) {
				continue;
			}

			fprintf(f, "\t\t\tcase '");
			escputc(i, f);
			fprintf(f, "':");

			/* non-unique states fall through */
			if (contains(state->edges, i + 1, state->edges[i].sl->state)) {
				fprintf(f, "\n");
				continue;
			}

			/* TODO: pad S%u out to maximum state width */
			if (state->edges[i].sl->state != state) {
				fprintf(f, " state = S%u; continue;\n", indexof(fsm, state->edges[i].sl->state));
			} else {
				fprintf(f, "             continue;\n");
			}
		}

		if (mode != NULL) {
			fprintf(f, "\t\t\tdefault:  state = S%u; continue;\n", indexof(fsm, mode));
		} else {
			fprintf(f, "\t\t\tdefault:  return TOK_ERROR;\n");	/* TODO: right? */
		}
	}

	fprintf(f, "\t\t\t}\n");
}

static void
stateenum(FILE *f, const struct fsm *fsm, struct fsm_state *sl)
{
	struct fsm_state *s;
	int i;

	fprintf(f, "\tenum {\n");
	fprintf(f, "\t\t");

	for (s = sl, i = 1; s != NULL; s = s->next, i++) {
		fprintf(f, "S%u", indexof(fsm, s));
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

static void
endstates(FILE *f, const struct fsm *fsm, struct fsm_state *sl)
{
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

		fprintf(f, "\tcase S%u: return %u;\n", indexof(fsm, s), indexof(fsm, s));
	}
	fprintf(f, "\tdefault: return EOF; /* unexpected EOF */\n");
	fprintf(f, "\t}\n");
}

static void
out_cfrag(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options)
{
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(fsm_isdfa(fsm));
	assert(f != NULL);
	assert(options != NULL);

	/* TODO: prerequisite that the FSM is a DFA */

	fprintf(f, "\t\tswitch (state) {\n");
	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "\t\tcase S%u:\n", indexof(fsm, s));
		singlecase(f, fsm, s);

		if (s->next != NULL) {
			fprintf(f, "\n");
		}
	}
	fprintf(f, "\t\t}\n");
}

void
fsm_out_c(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options)
{
	assert(fsm != NULL);
	assert(fsm_isdfa(fsm));
	assert(f != NULL);
	assert(options != NULL);

	/* TODO: prerequisite that the FSM is a DFA */

	/* TODO: pass in %s prefix (default to "fsm_") */

	if (options->fragment) {
		out_cfrag(fsm, f, options);
		return;
	}

	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "\n");
	fprintf(f, "extern int fsm_getc(void *opaque);\n");

	fprintf(f, "\n");

	fprintf(f, "int fsm_main(void *opaque) {\n");
	fprintf(f, "\tint c;\n");
	fprintf(f, "\n");

	/* enum of states */
	stateenum(f, fsm, fsm->sl);
	fprintf(f, "\n");

	/* start state */
	assert(fsm->start != NULL);
	fprintf(f, "\tstate = S%u;\n", indexof(fsm, fsm->start));
	fprintf(f, "\n");

	fprintf(f, "\twhile ((c = fsm_getc(opaque)) != EOF) {\n");
	out_cfrag(fsm, f, options);
	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	/* end states */
	endstates(f, fsm, fsm->sl);

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

