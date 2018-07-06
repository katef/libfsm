/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>

#include <print/esc.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include "libfsm/internal.h"

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

	assert(f != NULL);
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
				c_escputcharlit(f, opt, groups[j].ranges[k].start);
				fprintf(f, " ... ");
				c_escputcharlit(f, opt, groups[j].ranges[k].end);
				fprintf(f, ":");

				if (k + 1 < groups[j].n) {
					fprintf(f, "\n");
				}
			} else for (c = groups[j].ranges[k].start; c <= groups[j].ranges[k].end; c++) {
				fprintf(f, "\t\t\tcase ");
				c_escputcharlit(f, opt, c);
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
print_stateenum(FILE *f, size_t n)
{
	size_t i;

	assert(f != NULL);

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
fsm_print_cfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt,
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
				escputs(f, opt, c_escputc_str, ir->states[i].example);
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
fsm_print_c_complete(FILE *f, const struct ir *ir, const struct fsm_options *opt)
{
	const char *cp;

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

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
	print_stateenum(f, ir->n);
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

	(void) fsm_print_cfrag(f, ir, opt, cp,
		opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque);

	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	/* end states */
	endstates(f, opt, ir);
}

void
fsm_print_c(FILE *f, const struct fsm *fsm)
{
	struct ir *ir;
	const char *prefix;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

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
		fsm_print_c_complete(f, ir, fsm->opt);
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

	fsm_print_c_complete(f, ir, fsm->opt);

	fprintf(f, "}\n");
	fprintf(f, "\n");

	free_ir(ir);
}

