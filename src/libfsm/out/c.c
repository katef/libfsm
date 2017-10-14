/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/out.h>
#include <fsm/options.h>

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
escputc(const struct fsm_options *opt, int c, FILE *f)
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

	assert(opt != NULL);
	assert(c != FSM_EDGE_EPSILON);
	assert(f != NULL);

	if (opt->always_hex) {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

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
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

/* TODO: centralise */
static void
escputchar(const struct fsm_options *opt, int c, FILE *f)
{
	assert(opt != NULL);
	assert(f != NULL);

	if (opt->always_hex || c > SCHAR_MAX) {
		fprintf(f, "0x%02x", (unsigned char) c);
		return;
	}

	fprintf(f, "'");
	escputc(opt, c, f);
	fprintf(f, "'");
}

/* TODO: centralise, maybe with callback */
static int
escputs(const struct fsm_options *opt, FILE *f, const char *s)
{
	const char *p;
	int r, n;

	assert(opt != NULL);
	assert(f != NULL);
	assert(s != NULL);

	n = 0;

	for (p = s; *p != '\0'; p++) {
		r = escputc(opt, *p, f);
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
singlecase(FILE *f, const struct fsm *fsm,
	const char *cp,
	struct fsm_state *state,
	int (*leaf)(FILE *, const struct fsm *, const struct fsm_state *, const void *),
	const void *opaque)
{
	struct {
		struct fsm_state *state;
		unsigned int freq;
	} mode;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(cp != NULL);
	assert(f != NULL);
	assert(state != NULL);
	assert(leaf != NULL);

	/* no edges */
	{
		struct fsm_edge *e;
		struct set_iter it;

		e = set_first(state->edges, &it);
		if (!e || e->symbol > UCHAR_MAX) {
			fprintf(f, "\t\t\t");
			leaf(f, fsm, state, opaque);
			fprintf(f, "\n");
			return;
		}
	}

	/* all edges go to the same state */
	{
		if (fsm_iscomplete(fsm, state)) {
			mode.state = fsm_findmode(state, &mode.freq);
		} else {
			mode.state = NULL;
		}

		if (mode.state != NULL && mode.freq == UCHAR_MAX) {
			fprintf(f, "\t\t\t");
			if (mode.state != state) {
				fprintf(f, "state = S%u; ", indexof(fsm, mode.state));
			}
			fprintf(f, "break;\n");
			return;
		}
	}

	fprintf(f, "\t\t\tswitch ((unsigned char) %s) {\n", cp);

	/* usual case */
	{
		struct fsm_edge *e;
		struct set_iter it;

		for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
			struct fsm_state *s;

			if (e->symbol > UCHAR_MAX) {
				break;
			}

			if (set_empty(e->sl)) {
				continue;
			}

			s = set_only(e->sl);
			if (s == mode.state) {
				continue;
			}

			fprintf(f, "\t\t\tcase ");
			escputchar(fsm->opt, e->symbol, f);

			if (fsm->opt->case_ranges) {
				const struct fsm_edge *ne;
				enum fsm_edge_type p, q;
				struct set_iter ic, ir;

				ir = ic = it;
				p = q = e->symbol;
				ne = set_next(&ic);
				while (ne && ne->symbol - q == 1) {
					q = ne->symbol;
					ir = ic;
					ne = set_next(&ic);
				}

				if (q - p) {
					fprintf(f, " ... ");
					escputchar(fsm->opt, q, f);
					it = ir;
				}
			}

			fprintf(f, ":");

			/* non-unique states fall through */
			if (!fsm->opt->case_ranges && e->symbol <= UCHAR_MAX - 1) {
				const struct fsm_edge *ne;
				struct set_iter jt;

				ne = set_firstafter(state->edges, &jt, e);
				if (ne && ne->symbol == e->symbol + 1 && !set_empty(ne->sl)) {
					const struct fsm_state *ns;

					ns = set_only(ne->sl);
					if (ns != mode.state && ns == s) {
						fprintf(f, "\n");
						continue;
					}
				}
			}

			/* TODO: pad S%u out to maximum state width */
			if (s != state) {
				fprintf(f, " state = S%u;", indexof(fsm, s));
			}
			fprintf(f, " break;\n");

			/* TODO: if greedy, and fsm_isend(fsm, state->edges[i].sl->state) then:
				fprintf(f, "         return %s%s;\n", prefix.tok, state->edges[i].sl->state's token);
			 */
		}

		if (mode.state != NULL) {
			fprintf(f, "\t\t\tdefault: ");
			if (mode.state != state) {
				fprintf(f, "state = S%u; ", indexof(fsm, mode.state));
			}
			fprintf(f, "break;\n");
		} else {
			fprintf(f, "\t\t\tdefault:  ");
			leaf(f, fsm, state, opaque);
			fprintf(f, "\n");
		}
	}

	fprintf(f, "\t\t\t}\n");

	fprintf(f, "\t\t\tbreak;\n");
}

static void
out_stateenum(FILE *f, const struct fsm *fsm, struct fsm_state *sl)
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
fsm_out_cfrag(const struct fsm *fsm, FILE *f,
	const char *cp,
	int (*leaf)(FILE *, const struct fsm *, const struct fsm_state *, const void *),
	const void *opaque)
{
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(fsm_all(fsm, fsm_isdfa));
	assert(f != NULL);
	assert(cp != NULL);

	/* TODO: prerequisite that the FSM is a DFA */
	assert(fsm->start != NULL);

	fprintf(f, "\t\tswitch (state) {\n");
	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "\t\tcase S%u:", indexof(fsm, s));

		if (fsm->opt->comments) {
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
				escputs(fsm->opt, f, buf);
				fprintf(f, "%s\" */",
					n >= (int) sizeof buf - 1 ? "..." : "");
			}
		}
		fprintf(f, "\n");

		singlecase(f, fsm, cp, s, leaf, opaque);

		fprintf(f, "\n");
	}
	fprintf(f, "\t\tdefault:\n");
	fprintf(f, "\t\t\t; /* unreached */\n");
	fprintf(f, "\t\t}\n");

	return 0;
}

static void
fsm_out_c_complete(const struct fsm *fsm, FILE *f)
{
	const char *cp;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(fsm_all(fsm, fsm_isdfa));
	assert(f != NULL);

	if (fsm->opt->cp != NULL) {
		cp = fsm->opt->cp;
	} else {
		switch (fsm->opt->io) {
		case FSM_IO_GETC: cp = "c";  break;
		case FSM_IO_STR:  cp = "*p"; break;
		case FSM_IO_PAIR: cp = "*p"; break;
		}
	}

	/* enum of states */
	out_stateenum(f, fsm, fsm->sl);
	fprintf(f, "\n");

	/* start state */
	assert(fsm->start != NULL);
	fprintf(f, "\tstate = S%u;\n", indexof(fsm, fsm->start));
	fprintf(f, "\n");

	switch (fsm->opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "\twhile (c = fsm_getc(opaque), c != EOF) {\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tfor (p = s; *p != '\\0'; p++) {\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\tfor (p = b; *p != e; p++) {\n");
		break;
	}

	(void) fsm_out_cfrag(fsm, f, cp,
		fsm->opt->leaf != NULL ? fsm->opt->leaf : leaf, fsm->opt->leaf_opaque);

	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	/* end states */
	endstates(f, fsm, fsm->sl);
}

void
fsm_out_c(const struct fsm *fsm, FILE *f)
{
	const char *prefix;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(f != NULL);

	if (!fsm_all(fsm, fsm_isdfa)) {
		errno = EINVAL;
		return;
	}

	/* TODO: pass in %s prefix (default to "fsm_") */
	if (fsm->opt->prefix != NULL) {
		prefix = fsm->opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (fsm->opt->fragment) {
		fsm_out_c_complete(fsm, f);
		return;
	}

	fprintf(f, "#include <assert.h>\n");
	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "\n");

	fprintf(f, "int\n%smain", prefix);

	switch (fsm->opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "(int (*fsm_getc)(void *opaque), void *opaque)\n");
		fprintf(f, "{\n");
		fprintf(f, "\tint c;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(fsm_getc != NULL);\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "(const char *s)\n");
		fprintf(f, "{\n");
		fprintf(f, "\tconst char *p;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(s != NULL);\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "(const char *b, const char *e)\n");
		fprintf(f, "{\n");
		fprintf(f, "\tconst char *p;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(b != NULL);\n");
		fprintf(f, "\tassert(e != NULL);\n");
		fprintf(f, "\tassert(e > b);\n");
		fprintf(f, "\n");
		break;
	}

	fsm_out_c_complete(fsm, f);

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

