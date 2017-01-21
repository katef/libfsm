/* $Id$ */

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/out.h>

#include "libfsm/internal.h"
#include "libfsm/out.h"

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

/* TODO: centralise */
static int
escputc(int c, FILE *f)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ '\\', "\\\\" },
		{ '\'', "\\\'" },

		{ '\a', "\\a"  },
		{ '\b', "\\b"  },
		{ '\f', "\\f"  },
		{ '\n', "\\n"  },
		{ '\r', "\\r"  },
		{ '\t', "\\t"  },
		{ '\v', "\\v"  }
	};

	assert(c != FSM_EDGE_EPSILON);
	assert(f != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	/*
	 * Escaping '/' here is a lazy way to avoid keeping state when
	 * emitting '*', '/', since this is used to output example strings
	 * inside comments.
	 */

	if (!isprint((unsigned char) c) || c == '/') {
		return fprintf(f, "\\%03o", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

/* TODO: centralise, maybe with callback */
static int
escputs(FILE *f, const char *s)
{
	const char *p;
	int r, n;

	assert(f != NULL);
	assert(s != NULL);

	n = 0;

	for (p = s; *p != '\0'; p++) {
		r = escputc(*p, f);
		if (r == -1) {
			return -1;
		}

		n += r;
	}

	return n;
}

static int
leaf(FILE *f, const struct fsm *fsm, const struct fsm_state *state,
	const void *opaque)
{
	assert(fsm != NULL);
	assert(f != NULL);
	assert(state != NULL);
	assert(opaque == NULL);

	/* XXX: this should be FSM_UNKNOWN or something non-EOF,
	 * maybe user defined */
	fprintf(f, "return TOK_UNKNOWN;");

	return 0;
}

static void
singlecase(FILE *f, const struct fsm *fsm, struct fsm_state *state,
	int (*leaf)(FILE *, const struct fsm *, const struct fsm_state *, const void *),
	const void *opaque)
{
	int i;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(state != NULL);
	assert(leaf != NULL);

	{
		/* TODO: move this out into a count function */
		for (i = 0; i <= UCHAR_MAX; i++) {
			if (!set_empty(state->edges[i].sl)) {
				break;
			}
		}

		/* no edges */
		if (i > UCHAR_MAX) {
			fprintf(f, "\t\t\t");
			leaf(f, fsm, state, opaque);
			fprintf(f, "\n");
			return;
		}
	}

	fprintf(f, "\t\t\tswitch (c) {\n");

	/* usual case */
	{
		struct fsm_state *mode;

		mode = fsm_iscomplete(fsm, state) ? fsm_findmode(state) : NULL;

		for (i = 0; i <= UCHAR_MAX; i++) {
			struct set *set;
			struct set_iter it;
			struct fsm_state *s;

			set = state->edges[i].sl;
			if (set_empty(set)) {
				continue;
			}

			s = set_first(set, &it);
			assert(s != NULL);
			assert(!set_hasnext(&it));

			if (s == mode) {
				continue;
			}

			fprintf(f, "\t\t\tcase '");
			escputc(i, f);
			fprintf(f, "':");

			/* non-unique states fall through */
			if (i <= UCHAR_MAX - 1) {
				struct set *nset;
				struct fsm_state *ns;

				nset = state->edges[i + 1].sl;
				ns = set_first(nset, &it);

				if (!set_empty(nset) && ns != mode && ns == s) {
					fprintf(f, "\n");
					continue;
				}
			}

			/* TODO: pad S%u out to maximum state width */
			if (s != state) {
				fprintf(f, " state = S%u; continue;\n", indexof(fsm, s));
			} else {
				fprintf(f, "             continue;\n");
			}

			/* TODO: if greedy, and fsm_isend(fsm, state->edges[i].sl->state) then:
				fprintf(f, "         return %s%s;\n", prefix.tok, state->edges[i].sl->state's token);
			 */
		}

		if (mode != NULL) {
			fprintf(f, "\t\t\tdefault:  state = S%u; continue;\n", indexof(fsm, mode));
		} else {
			fprintf(f, "\t\t\tdefault:  ");
			leaf(f, fsm, state, opaque);
			fprintf(f, "\n");
		}
	}

	fprintf(f, "\t\t\t}\n");
}

void
fsm_out_stateenum(FILE *f, const struct fsm *fsm, struct fsm_state *sl)
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
	if (!fsm_has(fsm, fsm_isend)) {
		printf("\treturn EOF; /* unexpected EOF */\n");
		return;
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

int
fsm_out_cfrag(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options,
	int (*leaf)(FILE *, const struct fsm *, const struct fsm_state *, const void *),
	const void *opaque)
{
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(fsm_all(fsm, fsm_isdfa));
	assert(f != NULL);
	assert(options != NULL);

	/* TODO: prerequisite that the FSM is a DFA */

	fprintf(f, "\t\tswitch (state) {\n");
	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "\t\tcase S%u:", indexof(fsm, s));

		if (options->comments) {
			if (s == fsm->start) {
				fprintf(f, " /* start */");
			} else {
				char buf[50];
				int n;

				n = fsm_example(fsm, s, buf, sizeof buf);
				if (-1 == n) {
					perror("fsm_example");
					return -1;
				}

				fprintf(f, " /* e.g. \"");
				escputs(f, buf);
				fprintf(f, "%s\" */",
					n >= (int) sizeof buf - 1 ? "..." : "");
			}
		}
		fprintf(f, "\n");

		singlecase(f, fsm, s, leaf, opaque);

		if (s->next != NULL) {
			fprintf(f, "\n");
		}
	}
	fprintf(f, "\t\t}\n");

	return 0;
}

void
fsm_out_c(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options)
{
	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	if (!fsm_all(fsm, fsm_isdfa)) {
		errno = EINVAL;
		return;
	}

	/* TODO: pass in %s prefix (default to "fsm_") */

	if (options->fragment) {
		(void) fsm_out_cfrag(fsm, f, options, leaf, NULL);
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
	fsm_out_stateenum(f, fsm, fsm->sl);
	fprintf(f, "\n");

	/* start state */
	assert(fsm->start != NULL);
	fprintf(f, "\tstate = S%u;\n", indexof(fsm, fsm->start));
	fprintf(f, "\n");

	fprintf(f, "\twhile (c = fsm_getc(opaque), c != EOF) {\n");
	(void) fsm_out_cfrag(fsm, f, options, leaf, NULL);
	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	/* end states */
	endstates(f, fsm, fsm->sl);

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

