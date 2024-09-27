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
#include "libfsm/print.h"

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

// TODO: centralise vmc/c
static int
print_ids(FILE *f,
	enum fsm_ambig ambig, const struct fsm_state_metadata *state_metadata)
{
	switch (ambig) {
	case AMBIG_NONE:
		// TODO: for C99 we'd return bool
		fprintf(f, "return 1;");
		break;

	case AMBIG_ERROR:
// TODO: decide if we deal with this ahead of the call to print or not
		if (state_metadata->end_id_count > 1) {
			errno = EINVAL;
			return -1;
		}

		/* fallthrough */

	case AMBIG_EARLIEST:
		/*
		 * The libfsm api guarentees these ids are unique,
		 * and only appear once each, and are sorted.
		 */
		fprintf(f, "{\n");
		fprintf(f, "\t\t*id = %u;\n", state_metadata->end_ids[0]);
		fprintf(f, "\t\treturn 1;\n");
		fprintf(f, "\t}");
		break;

	case AMBIG_MULTIPLE:
		/*
		 * Here I would like to emit (static unsigned []) { 1, 2, 3 }
		 * but specifying a storage duration for compound literals
		 * is a compiler extension.
		 * So I'm emitting a static const variable declaration instead.
		 */

		fprintf(f, "{\n");
		fprintf(f, "\t\tstatic const unsigned a[] = { ");
		for (fsm_end_id_t i = 0; i < state_metadata->end_id_count; i++) {
			fprintf(f, "%u", state_metadata->end_ids[i]);
			if (i + 1 < state_metadata->end_id_count) {
				fprintf(f, ", ");
			}
		}
		fprintf(f, " };\n");
		fprintf(f, "\t\t*ids = a;\n");
		fprintf(f, "\t\t*count = %zu;\n", state_metadata->end_id_count);
		fprintf(f, "\t\treturn 1;\n");
		fprintf(f, "\t}");
		break;

	default:
		assert(!"unreached");
		abort();
	}

	return 0;
}

static int
default_accept(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);

	(void) lang_opaque;
	(void) hook_opaque;

	if (-1 == print_ids(f, opt->ambig, state_metadata)) {
		return -1;
	}

	return 0;
}

static int
default_reject(FILE *f, const struct fsm_options *opt,
	void *lang_opaque, void *hook_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);

	(void) lang_opaque;
	(void) hook_opaque;

	fprintf(f, "return 0;");

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

		if (groups[j].to != csi) {
			fprintf(f, " state = S%u;", groups[j].to);
		}
		fprintf(f, " break;\n");
	}
}

static int
print_case(FILE *f, const struct ir *ir,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const char *cp,
	struct ir_state *cs)
{
	assert(ir != NULL);
	assert(opt != NULL);
	assert(cp != NULL);
	assert(f != NULL);
	assert(cs != NULL);

	switch (cs->strategy) {
	case IR_NONE:
		fprintf(f, "\t\t\t");
		if (-1 == print_hook_reject(f, opt, hooks, default_reject, NULL)) {
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
		if (-1 == print_hook_reject(f, opt, hooks, default_reject, NULL)) {
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
		if (-1 == print_hook_reject(f, opt, hooks, default_reject, NULL)) {
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
print_endstates(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ir *ir)
{
	unsigned i;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);

	/* no end states */
	if (!ir_hasend(ir)) {
		fprintf(f, "\treturn 0;");
		if (opt->comments) {
			fprintf(f, " /* unexpected EOT */");
		}
		fprintf(f, "\n");
		return 0;
	}

	/* usual case */
	if (opt->comments) {
		fprintf(f, "\t/* end states */\n");
	}
	fprintf(f, "\tswitch (state) {\n");
	for (i = 0; i < ir->n; i++) {
		if (!ir->states[i].isend) {
			continue;
		}

		fprintf(f, "\tcase S%u: ", i);

		const struct fsm_state_metadata state_metadata = {
			.end_ids = ir->states[i].endids.ids,
			.end_id_count = ir->states[i].endids.count,
		};

		if (-1 == print_hook_accept(f, opt, hooks,
			&state_metadata,
			default_accept,
			NULL))
		{
			return -1;
		}

		if (-1 == print_hook_comment(f, opt, hooks,
			&state_metadata))
		{
			return -1;
		}

		fprintf(f, "\n");
	}

	/* unexpected EOT */
	fprintf(f, "\tdefault: ");
	if (-1 == print_hook_reject(f, opt, hooks, default_reject, NULL)) {
		return -1;
	}
	fprintf(f, "\n");
	fprintf(f, "\t}\n");

	return 0;
}

int
fsm_print_cfrag(FILE *f, const struct ir *ir,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const char *cp)
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
			} else if (i == ir->start && opt->comments) {
				fprintf(f, " /* start */");
			}
		}
		fprintf(f, "\n");

		if (-1 == print_case(f, ir, opt, hooks, cp, &ir->states[i])) {
			return -1;
		}

		fprintf(f, "\n");
	}
	fprintf(f, "\t\tdefault:\n");
	fprintf(f, "\t\t\t;");
	if (opt->comments) {
		fprintf(f, " /* unreached */");
	}
	fprintf(f, "\n");
	fprintf(f, "\t\t}\n");

	if (ferror(f)) {
		return -1;
	}

	return 0;
}

static int
fsm_print_c_body(FILE *f, const struct ir *ir,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks)
{
	const char *cp;

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	if (hooks->cp != NULL) {
		cp = hooks->cp;
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
		fprintf(f, "\twhile (c = fsm_getc(getc_opaque), c != EOF) {\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tfor (p = s; *p != '\\0'; p++) {\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\tfor (p = b; p != e; p++) {\n");
		break;
	}

	if (-1 == fsm_print_cfrag(f, ir, opt, hooks, cp)) {
		return -1;
	}

	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	/* end states */
	if (-1 == print_endstates(f, opt, hooks, ir)) {
		return -1;
	}

	return 0;
}

int
fsm_print_c(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ir *ir)
{
	const char *prefix;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(ir != NULL);

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (opt->fragment) {
		if (-1 == fsm_print_c_body(f, ir, opt, hooks)) {
			return -1;
		}
	} else {
		fprintf(f, "\n");

		fprintf(f, "int\n%smain", prefix);
		fprintf(f, "(");

		switch (opt->io) {
		case FSM_IO_GETC:
			fprintf(f, "int (*fsm_getc)(void *getc_opaque), void *getc_opaque");
			break;

		case FSM_IO_STR:
			fprintf(f, "const char *s");
			break;

		case FSM_IO_PAIR:
			fprintf(f, "const char *b, const char *e");
			break;
		}

		/*
		 * unsigned rather than fsm_end_id_t here, so the generated code
		 * doesn't depend on fsm.h
		 */
		switch (opt->ambig) {
		case AMBIG_NONE:
			break;

		case AMBIG_ERROR:
		case AMBIG_EARLIEST:
			fprintf(f, ",\n");
			fprintf(f, "\tconst unsigned *id");
			break;

		case AMBIG_MULTIPLE:
			fprintf(f, ",\n");
			fprintf(f, "\tconst unsigned **ids, size_t *count");
			break;

		default:
			assert(!"unreached");
			abort();
		}

		if (hooks->args != NULL) {
			fprintf(f, ",\n");
			fprintf(f, "\t");

			if (-1 == print_hook_args(f, opt, hooks, NULL, NULL)) {
				return -1;
			}
		}

		fprintf(f, ")\n");
		fprintf(f, "{\n");

		switch (opt->io) {
		case FSM_IO_GETC:
			if (ir->n > 0) {
				fprintf(f, "\tint c;\n");
				fprintf(f, "\n");
			}
			break;

		case FSM_IO_STR:
			if (ir->n > 0) {
				fprintf(f, "\tconst char *p;\n");
				fprintf(f, "\n");
			}
			break;

		case FSM_IO_PAIR:
			if (ir->n > 0) {
				fprintf(f, "\tconst char *p;\n");
				fprintf(f, "\n");
			}
			break;
		}

		if (ir->n == 0) {
			fprintf(f, "\treturn 0;");
			if (opt->comments) {
				fprintf(f, " /* no matches */");
			}
			fprintf(f, "\n");
		} else {
			if (-1 == fsm_print_c_body(f, ir, opt, hooks)) {
				return -1;
			}
		}

		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	return 0;
}

