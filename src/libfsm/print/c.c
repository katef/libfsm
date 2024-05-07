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
leaf(FILE *f, const struct fsm_end_ids *ids, const void *leaf_opaque)
{
	assert(f != NULL);
	assert(leaf_opaque == NULL);

	(void) ids;
	(void) leaf_opaque;

	/* XXX: this should be FSM_UNKNOWN or something non-EOF,
	 * maybe user defined */
	fprintf(f, "return -1; /* leaf */");

	return 0;
}

static void
print_ranges(FILE *f, const struct fsm_options *opt,
	const struct ir_range *ranges, size_t n)
{
	size_t k;
	size_t c;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ranges != NULL);

	for (k = 0; k < n; k++) {
		assert(ranges[k].end >= ranges[k].start);

		if (opt->case_ranges && ranges[k].end > ranges[k].start) {
			fprintf(f, "\t\t\tcase ");
			c_escputcharlit(f, opt, ranges[k].start);
			fprintf(f, " ... ");
			c_escputcharlit(f, opt, ranges[k].end);
			fprintf(f, ":");

			if (k + 1 < n) {
				fprintf(f, "\n");
			}
		} else for (c = ranges[k].start; c <= ranges[k].end; c++) {
			fprintf(f, "\t\t\tcase ");
			c_escputcharlit(f, opt, (char) c);
			fprintf(f, ":");

			if (k + 1 < n || c + 1 <= ranges[k].end) {
				fprintf(f, "\n");
			}
		}
	}
}

static void
print_groups(FILE *f, const struct fsm_options *opt,
	unsigned csi,
	const struct ir_group *groups, size_t n)
{
	size_t j;

	assert(f != NULL);
	assert(opt != NULL);
	assert(groups != NULL);

	for (j = 0; j < n; j++) {
		assert(groups[j].ranges != NULL);

		print_ranges(f, opt, groups[j].ranges, groups[j].n);

		/* TODO: pad S%u out to maximum state width */
		if (groups[j].to != csi) {
			fprintf(f, " state = S%u;", groups[j].to);
		}
		fprintf(f, " break;\n");

		/* TODO: if greedy, and fsm_isend(fsm, state->edges[i].sl->state) then:
			fprintf(f, "         return %s%s;\n", prefix.tok, state->edges[i].sl->state's token);
		 */
	}
}

static int
print_singlecase(FILE *f, const struct ir *ir, const struct fsm_options *opt,
	const char *cp,
	struct ir_state *cs,
	int (*leaf)(FILE *, const struct fsm_end_ids *ids, const void *leaf_opaque),
	const void *leaf_opaque)
{
	assert(ir != NULL);
	assert(opt != NULL);
	assert(cp != NULL);
	assert(f != NULL);
	assert(cs != NULL);
	assert(leaf != NULL);

	switch (cs->strategy) {
	case IR_NONE:
		fprintf(f, "\t\t\t");
		if (-1 == leaf(f, cs->end_ids, leaf_opaque)) {
			return -1;
		}
		fprintf(f, "\n");
		return 0;

	case IR_SAME:
		fprintf(f, "\t\t\t");
		if (cs->u.same.to != ir_indexof(ir, cs)) {
			fprintf(f, "state = S%u; ", cs->u.same.to);
		}
		fprintf(f, "break;\n");
		return 0;

	case IR_COMPLETE:
		fprintf(f, "\t\t\tswitch ((unsigned char) %s) {\n", cp);

		print_groups(f, opt, ir_indexof(ir, cs), cs->u.complete.groups, cs->u.complete.n);

		fprintf(f, "\t\t\t}\n");
		fprintf(f, "\t\t\tbreak;\n");
		return 0;

	case IR_PARTIAL:
		fprintf(f, "\t\t\tswitch ((unsigned char) %s) {\n", cp);

		print_groups(f, opt, ir_indexof(ir, cs), cs->u.partial.groups, cs->u.partial.n);

		fprintf(f, "\t\t\tdefault:  ");
		if (-1 == leaf(f, cs->end_ids, leaf_opaque)) {
			return -1;
		}
		fprintf(f, "\n");

		fprintf(f, "\t\t\t}\n");
		fprintf(f, "\t\t\tbreak;\n");
		return 0;

	case IR_DOMINANT:
		fprintf(f, "\t\t\tswitch ((unsigned char) %s) {\n", cp);

		print_groups(f, opt, ir_indexof(ir, cs), cs->u.dominant.groups, cs->u.dominant.n);

		fprintf(f, "\t\t\tdefault: ");
		if (cs->u.dominant.mode != ir_indexof(ir, cs)) {
			fprintf(f, "state = S%u; ", cs->u.dominant.mode);
		}
		fprintf(f, "break;\n");

		fprintf(f, "\t\t\t}\n");
		fprintf(f, "\t\t\tbreak;\n");
		return 0;

	case IR_ERROR:
		fprintf(f, "\t\t\tswitch ((unsigned char) %s) {\n", cp);

		print_groups(f, opt, ir_indexof(ir, cs), cs->u.error.groups, cs->u.error.n);

		print_ranges(f, opt, cs->u.error.error.ranges, cs->u.error.error.n);
		fprintf(f, " ");
		if (-1 == leaf(f, cs->end_ids, leaf_opaque)) {
			return -1;
		}
		fprintf(f, "\n");

		fprintf(f, "\t\t\tdefault: ");
		if (cs->u.error.mode != ir_indexof(ir, cs)) {
			fprintf(f, "state = S%u; ", cs->u.error.mode);
		}
		fprintf(f, "break;\n");

		fprintf(f, "\t\t\t}\n");
		fprintf(f, "\t\t\tbreak;\n");
		return 0;

	case IR_TABLE:
		errno = ENOTSUP;
		return -1;
	}

	fprintf(f, "\t\t\tswitch ((unsigned char) %s) {\n", cp);

	fprintf(f, "\t\t\t}\n");

	fprintf(f, "\t\t\tbreak;\n");

	return 0;
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

static int
endstates(FILE *f, const struct fsm_options *opt, const struct ir *ir)
{
	unsigned i;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);

	/* no end states */
	if (!ir_hasend(ir)) {
		fprintf(f, "\treturn -1; /* unexpected EOT */\n");
		return 0;
	}

	/* usual case */
	fprintf(f, "\t/* end states */\n");
	fprintf(f, "\tswitch (state) {\n");
	for (i = 0; i < ir->n; i++) {
		if (!ir->states[i].isend) {
			continue;
		}

		fprintf(f, "\tcase S%u: ", i);
		if (opt->endleaf != NULL) {
			if (-1 == opt->endleaf(f, ir->states[i].end_ids, opt->endleaf_opaque)) {
				return -1;
			}
		} else {
			fprintf(f, "return %u;", i);
		}
		fprintf(f, "\n");
	}
	fprintf(f, "\tdefault: return -1; /* unexpected EOT */\n");
	fprintf(f, "\t}\n");

	return 0;
}

int
fsm_print_cfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt,
	const char *cp,
	int (*leaf)(FILE *, const struct fsm_end_ids *ids, const void *leaf_opaque),
	const void *leaf_opaque)
{
	unsigned i;

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);
	assert(cp != NULL);

	fprintf(f, "\t\tswitch (state) {\n");
	for (i = 0; i < ir->n; i++) {
		fprintf(f, "\t\tcase S%u:", i);

		if (opt->comments) {
			if (ir->states[i].example != NULL) {
				fprintf(f, " /* e.g. \"");
				escputs(f, opt, c_escputc_str, ir->states[i].example);
				fprintf(f, "\" */");
			} else if (i == ir->start) {
				fprintf(f, " /* start */");
			}
		}
		fprintf(f, "\n");

		if (-1 == print_singlecase(f, ir, opt, cp, &ir->states[i], leaf, leaf_opaque)) {
			return -1;
		}

		fprintf(f, "\n");
	}
	fprintf(f, "\t\tdefault:\n");
	fprintf(f, "\t\t\t; /* unreached */\n");
	fprintf(f, "\t\t}\n");

	if (ferror(f)) {
		return -1;
	}

	return 0;
}

static int
fsm_print_c_body(FILE *f, const struct ir *ir, const struct fsm_options *opt)
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
	fprintf(f, "\tstate = S%u;\n", ir->start);
	fprintf(f, "\n");

	switch (opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "\twhile (c = fsm_getc(opaque), c != EOF) {\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tfor (p = s; *p != '\\0'; p++) {\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\tfor (p = b; p != e; p++) {\n");
		break;
	}

	if (-1 == fsm_print_cfrag(f, ir, opt, cp,
		opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque))
	{
		return -1;
	}

	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	/* end states */
	if (-1 == endstates(f, opt, ir)) {
		return -1;
	}

	return 0;
}

static int
fsm_print_c_complete(FILE *f, const struct ir *ir,
	const struct fsm_options *opt, const char *prefix)
{
	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

	if (opt->fragment) {
		if (-1 == fsm_print_c_body(f, ir, opt)) {
			return -1;
		}
	} else {
		fprintf(f, "\n");

		fprintf(f, "int\n%smain", prefix);

		switch (opt->io) {
		case FSM_IO_GETC:
			fprintf(f, "(int (*fsm_getc)(void *opaque), void *opaque)\n");
			fprintf(f, "{\n");
			if (ir->n > 0) {
				fprintf(f, "\tint c;\n");
				fprintf(f, "\n");
			}
			break;

		case FSM_IO_STR:
			fprintf(f, "(const char *s, void *opaque)\n");
			fprintf(f, "{\n");
			if (ir->n > 0) {
				fprintf(f, "\tconst char *p;\n");
				fprintf(f, "\n");
			}
			break;

		case FSM_IO_PAIR:
			fprintf(f, "(const char *b, const char *e, void *opaque)\n");
			fprintf(f, "{\n");
			if (ir->n > 0) {
				fprintf(f, "\tconst char *p;\n");
				fprintf(f, "\n");
			}
			break;
		}

		if (ir->n == 0) {
			fprintf(f, "\treturn -1; /* no matches */\n");
		} else {
			if (-1 == fsm_print_c_body(f, ir, opt)) {
				return -1;
			}
		}

		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	if (ferror(f)) {
		return -1;
	}

	return 0;
}

int
fsm_print_c(FILE *f, const struct fsm *fsm)
{
	struct ir *ir;
	const char *prefix;
	int r;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return -1;
	}

	/* henceforth, no function should be passed struct fsm *, only the ir and options */

	if (fsm->opt->prefix != NULL) {
		prefix = fsm->opt->prefix;
	} else {
		prefix = "fsm_";
	}

	r = fsm_print_c_complete(f, ir, fsm->opt, prefix);

	free_ir(fsm, ir);

	return r;
}

