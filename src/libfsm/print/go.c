/*
 * Copyright 2008-2020 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
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

#include "libfsm/vm/vm.h"

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
		assert(!"unreached");
		return NULL;
	}
}

static int
print_ids(FILE *f,
	enum fsm_ambig ambig, const fsm_end_id_t *ids, size_t count)
{
	switch (ambig) {
	case AMBIG_NONE:
		break;

	case AMBIG_ERROR:
// TODO: decide if we deal with this ahead of the call to print or not
		if (count > 1) {
			errno = EINVAL;
			return -1;
		}

		fprintf(f, ", %u;", ids[0]);
		break;

	case AMBIG_EARLIEST:
		/*
		 * The libfsm api guarentees these ids are unique,
		 * and only appear once each, and are sorted.
		 */
		fprintf(f, ", %u;", ids[0]);
		break;

	case AMBIG_MULTIPLE:
		assert(!"unimplemented");
		abort();

	default:
		assert(!"unreached");
		abort();
	}

	return 0;
}

static int
default_accept(FILE *f, const struct fsm_options *opt,
	const fsm_end_id_t *ids, size_t count,
	void *lang_opaque, void *hook_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);

	(void) lang_opaque;
	(void) hook_opaque;

	fprintf(f, "return true");

	if (-1 == print_ids(f, opt->ambig, ids, count)) {
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

	fprintf(f, "{\n\t\treturn false\n\t}\n");

    return 0;
}

static void
print_label(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	fprintf(f, "l%" PRIu32 ":", op->index);

	if (op->example != NULL) {
		fprintf(f, " // e.g. \"");
		/* Go's string escape rules are a superset of C's. */
		escputs(f, opt, c_escputc_str, op->example);
		fprintf(f, "\"");
	}
}

static void
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	if (op->cmp == VM_CMP_ALWAYS) {
		return;
	}

	fprintf(f, "if data[idx] %s ", cmp_operator(op->cmp));
	/* Go's character escapes are a superset of C's. */
	c_escputcharlit(f, opt, op->cmp_arg);
	fprintf(f, " ");
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

	case VM_END_SUCC:
		fprintf(f, "{\n");
		fprintf(f, "\t\t");

		if (-1 == print_hook_accept(f, opt, hooks,
			op->endids.ids, op->endids.count,
			default_accept,
			NULL))
		{
			return -1;
		}

		fprintf(f, "\n\t}\n");

		return 0;

	default:
		assert(!"unreached");
		abort();
	}
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "{\n\t\tgoto l%" PRIu32 "\n\t}\n", op->u.br.dest_arg->index);
}

static void
print_fetch(FILE *f, const struct fsm_options *opt)
{
	switch (opt->io) {
	case FSM_IO_STR:
	case FSM_IO_PAIR:
		fprintf(f, "if idx++; idx >= uint(len(data)) ");
		break;

	default:
		assert(!"unreached");
	}
}

/* TODO: eventually to be non-static */
static int
fsm_print_gofrag(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	struct dfavm_op_ir *ops,
	const char *cp)
{
	struct dfavm_op_ir *op;

	assert(f != NULL);
	assert(opt != NULL);
	assert(cp != NULL);

	/* TODO: we'll need to heed cp for e.g. lx's codegen */
	(void) cp;

	/*
	 * We only output labels for ops which are branched to. This gives
	 * gaps in the sequence for ops which don't need a label.
	 * So here we renumber just the ones we use.
	 */
	{
		uint32_t l;

		l = 0;

		for (op = ops; op != NULL; op = op->next) {
			if (op->num_incoming > 0) {
				op->index = l++;
			}
		}
	}

	/*
	 * Only declare variables if we're actually going to use them.
	 */
	if (ops->cmp == VM_CMP_ALWAYS && ops->instr == VM_OP_STOP) {
		assert(ops->next == NULL);
	} else {
		switch (opt->io) {
		case FSM_IO_PAIR:
			if (ops != NULL) {
				/* start idx at -1 unsigned so after first increment we're correct at index 0 */
				fprintf(f, "\tvar idx = ^uint(0)\n");
				fprintf(f, "\n");
			}
			break;

		case FSM_IO_STR:
			if (ops != NULL) {
				/* start idx at -1 unsigned so after first increment we're correct at index 0 */
				fprintf(f, "\tvar idx = ^uint(0)\n");
				fprintf(f, "\n");
			}
			break;

		default:
			assert(!"unreached");
			break;
		}
	}

	for (op = ops; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			print_label(f, op, opt);
			fprintf(f, "\n");
		}

		fprintf(f, "\t");

		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op, opt);
			print_end(f, op, opt, hooks, op->u.stop.end_bits);
			break;

		case VM_OP_FETCH:
			print_fetch(f, opt);
			print_end(f, op, opt, hooks, op->u.fetch.end_bits);
			break;

		case VM_OP_BRANCH:
			print_cond(f, op, opt);
			print_branch(f, op);
			break;

		default:
			assert(!"unreached");
			break;
		}

		fprintf(f, "\n");
	}

	return 0;
}

int
fsm_print_go(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	struct dfavm_op_ir *ops)
{
	const char *prefix;
	const char *package_prefix;

	/* TODO: currently unused, but must be non-NULL */
	const char *cp = "";

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (opt->package_prefix != NULL) {
		package_prefix = opt->package_prefix;
	} else {
		package_prefix = prefix;
	}

	if (opt->fragment) {
		if (-1 == fsm_print_gofrag(f, opt, hooks, ops, cp)) {
			return -1;
		}
	} else {
		fprintf(f, "package %sfsm\n", package_prefix);
		fprintf(f, "\n");

		fprintf(f, "func %sMatch", prefix);

		switch (opt->io) {
		case FSM_IO_PAIR:
			fprintf(f, "(data []byte)");
			break;

		case FSM_IO_STR:
			fprintf(f, "(data string)");
			break;

		default:
			errno = ENOTSUP;
			return -1;
		}

		if (hooks->args != NULL) {
			fprintf(f, ", ");
		
			if (-1 == print_hook_args(f, opt, hooks, NULL, NULL)) {
				return -1;
			}
		}

		fprintf(f, " ");

		switch (opt->ambig) {
		case AMBIG_NONE:
			fprintf(f, "bool");
			break;
	
		case AMBIG_ERROR:
		case AMBIG_EARLIEST:
			fprintf(f, "(bool, uint)");
			break;

		case AMBIG_MULTIPLE:
			// TODO: fprintf(f, "(bool, uint[])");
			errno = ENOTSUP;
			return -1;
		
		default:
			assert(!"unreached");
			abort();
		}	

		fprintf(f, " {\n");

		if (-1 == fsm_print_gofrag(f, opt, hooks, ops, cp)) {
			return -1;
		}

		fprintf(f, "}\n");
	}

	return 0;
}

