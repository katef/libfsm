/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
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

#include "ir.h"

static unsigned int
ir_indexof(const struct ir *ir, const struct ir_state *cs)
{
	assert(ir != NULL);
	assert(cs != NULL);
	assert(cs >= ir->states);

	return cs - ir->states;
}

static int
ir_hasend(const struct ir *ir)
{
	size_t i;

	assert(ir != NULL);
	assert(ir->states != NULL);

	for (i = 0; i < ir->n; i++) {
		if (ir->states[i].isend) {
			return 1;
		}
	}

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
leaf(FILE *f, const void *state_opaque, const void *leaf_opaque)
{
	assert(f != NULL);
	assert(leaf_opaque == NULL);

	(void) state_opaque;
	(void) leaf_opaque;

	/* XXX: this should be FSM_UNKNOWN or something non-EOF,
	 * maybe user defined */
	fprintf(f, "return TOK_UNKNOWN;");

	return 0;
}

static void
out_groups(FILE *f, const struct ir *ir, const struct fsm_options *opt,
	const struct ir_state *cs,
	const struct ir_group *groups, size_t n)
{
	size_t j, k;

	assert(ir != NULL);
	assert(opt != NULL);
	assert(cs != NULL);
	assert(groups != NULL);

	for (j = 0; j < n; j++) {
		size_t c;

		assert(groups[j].ranges != NULL);

		for (k = 0; k < groups[j].n; k++) {
			assert(groups[j].ranges[k].end >= groups[j].ranges[k].start);

			if (opt->case_ranges && groups[j].ranges[k].end > groups[j].ranges[k].start) {
				fprintf(f, "\t\t\tcase ");
				escputchar(opt, groups[j].ranges[k].start, f);
				fprintf(f, " ... ");
				escputchar(opt, groups[j].ranges[k].end, f);
				fprintf(f, ":");

				if (k + 1 < groups[j].n) {
					fprintf(f, "\n");
				}
			} else for (c = groups[j].ranges[k].start; c <= groups[j].ranges[k].end; c++) {
				fprintf(f, "\t\t\tcase ");
				escputchar(opt, c, f);
				fprintf(f, ":");

				if (k + 1 < groups[j].n || c + 1 <= groups[j].ranges[k].end) {
					fprintf(f, "\n");
				}
			}
		}

		/* TODO: pad S%u out to maximum state width */
		if (groups[j].to != cs) {
			fprintf(f, " state = S%u;", ir_indexof(ir, groups[j].to));
		}
		fprintf(f, " break;\n");

		/* TODO: if greedy, and fsm_isend(fsm, state->edges[i].sl->state) then:
			fprintf(f, "         return %s%s;\n", prefix.tok, state->edges[i].sl->state's token);
		 */
	}
}

static void
out_singlecase(FILE *f, const struct ir *ir, const struct fsm_options *opt,
	const char *cp,
	struct ir_state *cs,
	int (*leaf)(FILE *, const void *state_opaque, const void *leaf_opaque),
	const void *leaf_opaque)
{
	assert(ir != NULL);
	assert(opt != NULL);
	assert(cp != NULL);
	assert(f != NULL);
	assert(cs != NULL);
	assert(leaf != NULL);

	switch (cs->strategy) {
	case IR_JUMP:
		/* TODO */
		abort();

	case IR_NONE:
		fprintf(f, "\t\t\t");
		leaf(f, cs->opaque, leaf_opaque);
		fprintf(f, "\n");
		return;

	case IR_SAME:
		fprintf(f, "\t\t\t");
		if (cs->u.same.to != cs) {
			fprintf(f, "state = S%u; ", ir_indexof(ir, cs->u.same.to));
		}
		fprintf(f, "break;\n");
		return;

	case IR_MANY:
		fprintf(f, "\t\t\tswitch ((unsigned char) %s) {\n", cp);

		out_groups(f, ir, opt, cs, cs->u.many.groups, cs->u.many.n);

		fprintf(f, "\t\t\tdefault:  ");
		leaf(f, cs->opaque, leaf_opaque);
		fprintf(f, "\n");

		fprintf(f, "\t\t\t}\n");
		fprintf(f, "\t\t\tbreak;\n");
		return;

	case IR_MODE:
		fprintf(f, "\t\t\tswitch ((unsigned char) %s) {\n", cp);

		out_groups(f, ir, opt, cs, cs->u.mode.groups, cs->u.mode.n);

		fprintf(f, "\t\t\tdefault: ");
		if (cs->u.mode.mode != cs) {
			fprintf(f, "state = S%u; ", ir_indexof(ir, cs->u.mode.mode));
		}
		fprintf(f, "break;\n");

		fprintf(f, "\t\t\t}\n");
		fprintf(f, "\t\t\tbreak;\n");
		return;
	}

	fprintf(f, "\t\t\tswitch ((unsigned char) %s) {\n", cp);

	fprintf(f, "\t\t\t}\n");

	fprintf(f, "\t\t\tbreak;\n");
}

static void
out_stateenum(FILE *f, size_t n)
{
	size_t i;

	fprintf(f, "\tenum {\n");
	fprintf(f, "\t\t");

	for (i = 0; i < n; i++) {
		fprintf(f, "S%u", (unsigned) i);
		if (i + 1 < n) {
			fprintf(f, ", ");
		}

		if (i + 1 < n && (i + 1) % 10 == 0) {
			fprintf(f, "\n");
			fprintf(f, "\t\t");
		}
	}

	fprintf(f, "\n");
	fprintf(f, "\t} state;\n");
}

static void
endstates(FILE *f, const struct fsm_options *opt, const struct ir *ir)
{
	size_t i;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);

	/* no end states */
	if (!ir_hasend(ir)) {
		printf("\treturn EOF; /* unexpected EOF */\n");
		return;
	}

	/* usual case */
	fprintf(f, "\t/* end states */\n");
	fprintf(f, "\tswitch (state) {\n");
	for (i = 0; i < ir->n; i++) {
		if (!ir->states[i].isend) {
			continue;
		}

		fprintf(f, "\tcase S%u: ", ir_indexof(ir, &ir->states[i]));
		if (opt->endleaf != NULL) {
			opt->endleaf(f, ir->states[i].opaque, opt->endleaf_opaque);
		} else {
			fprintf(f, "return %u;", ir_indexof(ir, &ir->states[i]));
		}
		fprintf(f, "\n");
	}
	fprintf(f, "\tdefault: return EOF; /* unexpected EOF */\n");
	fprintf(f, "\t}\n");
}

int
fsm_out_cfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt,
	const char *cp,
	int (*leaf)(FILE *, const void *state_opaque, const void *leaf_opaque),
	const void *leaf_opaque)
{
	size_t i;

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);
	assert(cp != NULL);
	assert(ir->start != NULL);

	fprintf(f, "\t\tswitch (state) {\n");
	for (i = 0; i < ir->n; i++) {
		fprintf(f, "\t\tcase S%u:", ir_indexof(ir, &ir->states[i]));

		if (opt->comments) {
			if (ir->states[i].example != NULL) {
				fprintf(f, " /* e.g. \"");
				escputs(opt, f, ir->states[i].example);
				fprintf(f, "\" */");
			} else if (&ir->states[i] == ir->start) {
				fprintf(f, " /* start */");
			}
		}
		fprintf(f, "\n");

		out_singlecase(f, ir, opt, cp, &ir->states[i], leaf, leaf_opaque);

		fprintf(f, "\n");
	}
	fprintf(f, "\t\tdefault:\n");
	fprintf(f, "\t\t\t; /* unreached */\n");
	fprintf(f, "\t\t}\n");

	return 0;
}

static void
fsm_out_c_complete(const struct ir *ir, const struct fsm_options *opt, FILE *f)
{
	const char *cp;

	assert(ir != NULL);
	assert(opt != NULL);
	assert(f != NULL);

	if (opt->cp != NULL) {
		cp = opt->cp;
	} else {
		switch (opt->io) {
		case FSM_IO_GETC: cp = "c";  break;
		case FSM_IO_STR:  cp = "*p"; break;
		case FSM_IO_PAIR: cp = "*p"; break;
		}
	}

	/* enum of states */
	out_stateenum(f, ir->n);
	fprintf(f, "\n");

	/* start state */
	assert(ir->start != NULL);
	fprintf(f, "\tstate = S%u;\n", ir_indexof(ir, ir->start));
	fprintf(f, "\n");

	switch (opt->io) {
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

	(void) fsm_out_cfrag(f, ir, opt, cp,
		opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque);

	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	/* end states */
	endstates(f, opt, ir);
}

void
fsm_out_c(const struct fsm *fsm, FILE *f)
{
	struct ir *ir;
	const char *prefix;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(f != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return;
	}

	/* henceforth, no function should be passed struct fsm *, only the ir and options */

	if (fsm->opt->prefix != NULL) {
		prefix = fsm->opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (fsm->opt->fragment) {
		fsm_out_c_complete(ir, fsm->opt, f);
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

	fsm_out_c_complete(ir, fsm->opt, f);

	fprintf(f, "}\n");
	fprintf(f, "\n");

	free_ir(ir);
}

