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

#include "libfsm/vm/vm.h"

#include "ir.h"

static int
print_leaf(FILE *f, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque)
{
	assert(f != NULL);
	assert(leaf_opaque == NULL);

	(void) ids;
	(void) count;
	(void) leaf_opaque;

	/* XXX: this should be FSM_UNKNOWN or something non-EOF,
	 * maybe user defined */
	fprintf(f, "return TOK_UNKNOWN;");

	return 0;
}

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

static void
print_label(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	fprintf(f, "l%" PRIu32 ":", op->index);

	if (op->ir_state->example != NULL) {
		fprintf(f, " // e.g. \"");
		/* Go's string escape rules are a superset of C's. */
		escputs(f, opt, c_escputc_str, op->ir_state->example);
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
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
	const struct ir_state *ir_states,
	enum dfavm_op_end end_bits)
{
	if (end_bits == VM_END_FAIL) {
		fprintf(f, "{\n\t\treturn -1\n\t}\n");
		return 0;
	}

	fprintf(f, "{\n");
	fprintf(f, "\t\t");

	if (opt->endleaf != NULL) {
		if (-1 == opt->endleaf(f,
			op->ir_state->endids.ids, op->ir_state->endids.count,
			opt->endleaf_opaque))
		{
			return -1;
		}
	} else {
		fprintf(f, "return %td", op->ir_state - ir_states);
	}

	fprintf(f, "\n\t}\n");

	return 0;
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
fsm_print_gofrag(FILE *f, const struct fsm_options *opt,
	const struct ir *ir, struct dfavm_op_ir *ops,
	const char *cp,
	int (*leaf)(FILE *, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque),
	const void *leaf_opaque)
{
	struct dfavm_op_ir *op;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);
	assert(cp != NULL);

	/* TODO: we don't currently have .opaque information attached to struct dfavm_op_ir.
	 * We'll need that in order to be able to use the leaf callback here. */
	(void) leaf;
	(void) leaf_opaque;

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
			if (ir->n > 0) {
				/* start idx at -1 unsigned so after first increment we're correct at index 0 */
				fprintf(f, "\tvar idx = ^uint(0)\n");
				fprintf(f, "\n");
			}
			break;

		case FSM_IO_STR:
			if (ir->n > 0) {
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
			print_end(f, op, opt, ir->states, op->u.stop.end_bits);
			break;

		case VM_OP_FETCH:
			print_fetch(f, opt);
			print_end(f, op, opt, ir->states, op->u.fetch.end_bits);
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
fsm_print_go(FILE *f, const struct fsm_options *opt,
	const struct ir *ir, struct dfavm_op_ir *ops)
{
	int (*leaf)(FILE *f, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque);
	const char *prefix;
	const char *package_prefix;

	/* TODO: currently unused, but must be non-NULL */
	const char *cp = "";

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (opt->leaf != NULL) {
		leaf = opt->leaf;
	} else {
		leaf = print_leaf;
	}

	if (opt->package_prefix != NULL) {
		package_prefix = opt->package_prefix;
	} else {
		package_prefix = prefix;
	}

	if (opt->fragment) {
		if (-1 == fsm_print_gofrag(f, opt, ir, ops, cp,
			leaf, opt->leaf_opaque))
		{
			return -1;
		}
	} else {
		fprintf(f, "package %sfsm\n", package_prefix);
		fprintf(f, "\n");

		fprintf(f, "func %sMatch", prefix);

		switch (opt->io) {
		case FSM_IO_PAIR:
			fprintf(f, "(data []byte) int {\n");
			break;

		case FSM_IO_STR:
			fprintf(f, "(data string) int {\n");
			break;

		default:
			errno = ENOTSUP;
			return -1;
		}

		if (-1 == fsm_print_gofrag(f, opt, ir, ops, cp,
			leaf, opt->leaf_opaque))
		{
			return -1;
		}

		fprintf(f, "}\n");
	}

	return 0;
}

