/*
 * Copyright 2008-2020 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <inttypes.h>

#include <print/esc.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/vm.h>

#include "libfsm/internal.h"
#include "libfsm/print.h"

#include "libfsm/vm/retlist.h"
#include "libfsm/vm/vm.h"

#define START UINT32_MAX

static const char *
cmp_operator(int cmp)
{
	switch (cmp) {
	case VM_CMP_LT: return "<";
	case VM_CMP_LE: return "<=";
	case VM_CMP_EQ: return "==";
	case VM_CMP_GE: return ">=";
	case VM_CMP_GT: return ">";
	case VM_CMP_NE: return "!=";

	case VM_CMP_ALWAYS:
	default:
		assert("unreached");
		return NULL;
	}
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

	switch (opt->ambig) {
	case AMBIG_NONE:
		/* awk doesn't have a boolean type */
		fprintf(f, "return 1;");
		break;

	case AMBIG_ERROR:
// TODO: decide if we deal with this ahead of the call to print or not
		if (state_metadata->end_id_count > 1) {
			errno = EINVAL;
			return -1;
		}

		fprintf(f, "return %u;", state_metadata->end_ids[0]);
		break;

	case AMBIG_EARLIEST:
		/*
		 * The libfsm api guarentees these ids are unique,
		 * and only appear once each, and are sorted.
		 */
		fprintf(f, "return %u;", state_metadata->end_ids[0]);
		break;

	case AMBIG_MULTIPLE:
		/* awk can't return an array */
		fprintf(f, "return \"");

		for (size_t i = 0; i < state_metadata->end_id_count; i++) {
			fprintf(f, "%u", state_metadata->end_ids[i]);

			if (i < state_metadata->end_id_count - 1) {
				fprintf(f, ",");
			}
		}

		fprintf(f, "\"");
		break;

	default:
		assert(!"unreached");
		abort();
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
print_label(FILE *f, const struct dfavm_op_ir *op)
{
	if (op->index == START) {
		fprintf(f, "\"\"");
	} else {
		fprintf(f, "%" PRIu32, op->index);
	}
}

static void
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	if (op->cmp == VM_CMP_ALWAYS) {
		return;
	}

	fprintf(f, "if (c %s ", cmp_operator(op->cmp));
	awk_escputcharlit(f, opt, op->cmp_arg);
	fprintf(f, ") ");
}

static int
print_end(FILE *f, const struct dfavm_op_ir *op,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	enum dfavm_op_end end_bits)
{
	switch (end_bits) {
	case VM_END_FAIL:
		return print_hook_reject(f, opt, hooks, default_reject, NULL);

	case VM_END_SUCC:;
		struct fsm_state_metadata state_metadata = {
			.end_ids = op->ret->ids,
			.end_id_count = op->ret->count,
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

		return 0;

	default:
		assert(!"unreached");
		abort();
	}
}

static void
print_jump(FILE *f, const struct dfavm_op_ir *op, const char *prefix)
{
	fprintf(f, "return %smain(s, ", prefix);
	print_label(f, op);
	fprintf(f, ")");
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op, const char *prefix)
{
	print_jump(f, op->u.br.dest_arg, prefix);
}

/* TODO: eventually to be non-static */
static int
fsm_print_awkfrag(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops,
	const char *cp, const char *prefix)
{
	struct dfavm_op_ir *op;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(retlist != NULL);
	assert(cp != NULL);
	assert(prefix != NULL);

	/* TODO: we'll need to heed cp for e.g. lx's codegen */
	(void) cp;

	/*
	 * We only output labels for ops which are branched to. This gives
	 * gaps in the sequence for ops which don't need a label.
	 * So here we renumber just the ones we use.
	 */
	{
		uint32_t l;

		l = START;

		for (op = ops; op != NULL; op = op->next) {
			if (op == ops|| op->num_incoming > 0) {
				op->index = l++;
			}
		}
	}

	fprintf(f, "    switch (l) {\n");

	for (op = ops; op != NULL; op = op->next) {
		if (op == ops|| op->num_incoming > 0) {
			if (op != ops) {
				fprintf(f, "\n");
			}

			fprintf(f, "    case ");
			print_label(f, op);
			fprintf(f, ":");

			if (op->example != NULL) {
				fprintf(f, " /* e.g. \"");
				escputs(f, opt, c_escputc_str, op->example);
				fprintf(f, "\" */");
			}

			fprintf(f, "\n");
		}

		fprintf(f, "        ");

		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op, opt);
			if (-1 == print_end(f, op, opt, hooks, op->u.stop.end_bits)) {
				return -1;
			}
			fprintf(f, ";");
			break;

		case VM_OP_FETCH: {
			fprintf(f, "if (s == \"\") ");
			if (-1 == print_end(f, op, opt, hooks, op->u.fetch.end_bits)) {
				return -1;
			}
			fprintf(f, "\n");

			fprintf(f, "        ");
			fprintf(f, "c = substr(s, 1, 1)");
			fprintf(f, "\n");

			fprintf(f, "        ");
			fprintf(f, "s = substr(s, 2)");
			fprintf(f, "\n");

			break;
		}

		case VM_OP_BRANCH:
			print_cond(f, op, opt);
			print_branch(f, op, prefix);
			break;

		default:
			assert(!"unreached");
			break;
		}

		fprintf(f, "\n");
	}

	fprintf(f, "    }\n");

	return 0;
}

int
fsm_print_awk(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	const char *prefix;
	const char *cp;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(retlist != NULL);

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (hooks->cp != NULL) {
		cp = hooks->cp;
	} else {
		cp = "c"; /* XXX */
	}

	if (opt->fragment) {
		if (-1 == fsm_print_awkfrag(f, opt, hooks, retlist, ops, cp, prefix)) {
			return -1;
		}
	} else {
		fprintf(f, "\n");

		fprintf(f, "# generated\n");
		fprintf(f, "function %smain(", prefix);

		switch (opt->io) {
		case FSM_IO_STR:
			fprintf(f, "s");
			break;

		case FSM_IO_GETC:
		case FSM_IO_PAIR:
		default:
			errno = ENOTSUP;
			return -1;
		}

		fprintf(f, ",    l, c) {\n");

		if (-1 == fsm_print_awkfrag(f, opt, hooks, retlist, ops, cp, prefix)) {
			return -1;
		}

		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	return 0;
}

