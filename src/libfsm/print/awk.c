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

#include "libfsm/vm/vm.h"

#include "ir.h"

#define START UINT32_MAX

static int
leaf(FILE *f, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque)
{
	assert(f != NULL);
	assert(leaf_opaque == NULL);

	(void) leaf_opaque;
	(void) ids;
	(void) count;

	/* XXX: this should be FSM_UNKNOWN or something non-EOF,
	 * maybe user defined */
	fprintf(f, "return -1;");

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
		assert("unreached");
		return NULL;
	}
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
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
	enum dfavm_op_end end_bits, const struct ir *ir)
{
	(void) ir;

	if (end_bits == VM_END_FAIL) {
		fprintf(f, "return -1");
		return 0;
	}

	if (opt->endleaf != NULL) {
		if (-1 == opt->endleaf(f,
			op->ir_state->endids.ids, op->ir_state->endids.count,
			opt->endleaf_opaque))
		{
			return -1;
		}
	} else {
		size_t i;

		assert(f != NULL);

		/* awk can't return an array */
		fprintf(f, "return \"");

		for (i = 0; i < op->ir_state->endids.count; i++) {
			fprintf(f, "%u", op->ir_state->endids.ids[i]);

			if (i < op->ir_state->endids.count - 1) {
				fprintf(f, ",");
			}
		}

		fprintf(f, "\"");
	}

	return 0;
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
fsm_print_awkfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt,
	const char *cp, const char *prefix,
	int (*leaf)(FILE *, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque),
	const void *leaf_opaque)
{
	static const struct dfavm_assembler_ir zero;
	struct dfavm_assembler_ir a;
	struct dfavm_op_ir *op;

	static const struct fsm_vm_compile_opts vm_opts = {
		FSM_VM_COMPILE_DEFAULT_FLAGS,
		FSM_VM_COMPILE_VM_V1,
		NULL
	};

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);
	assert(cp != NULL);
	assert(prefix != NULL);

	a = zero;

	/* TODO: we don't currently have .opaque information attached to struct dfavm_op_ir.
	 * We'll need that in order to be able to use the leaf callback here. */
	(void) leaf;
	(void) leaf_opaque;

	/* TODO: we'll need to heed cp for e.g. lx's codegen */
	(void) cp;

	if (!dfavm_compile_ir(&a, ir, vm_opts)) {
		return -1;
	}

	/*
	 * We only output labels for ops which are branched to. This gives
	 * gaps in the sequence for ops which don't need a label.
	 * So here we renumber just the ones we use.
	 */
	{
		uint32_t l;

		l = START;

		for (op = a.linked; op != NULL; op = op->next) {
			if (op == a.linked || op->num_incoming > 0) {
				op->index = l++;
			}
		}
	}

	fprintf(f, "    switch (l) {\n");

	for (op = a.linked; op != NULL; op = op->next) {
		if (op == a.linked || op->num_incoming > 0) {
			if (op != a.linked) {
				fprintf(f, "\n");
			}

			fprintf(f, "    case ");
			print_label(f, op);
			fprintf(f, ":");

			if (op->ir_state != NULL && op->ir_state->example != NULL) {
				fprintf(f, " /* e.g. \"");
				escputs(f, opt, c_escputc_str, op->ir_state->example);
				fprintf(f, "\" */");
			}

			fprintf(f, "\n");
		}

		fprintf(f, "        ");

		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op, opt);
			if (-1 == print_end(f, op, opt, op->u.stop.end_bits, ir)) {
				return -1;
			}
			fprintf(f, ";");
			break;

		case VM_OP_FETCH: {
			fprintf(f, "if (s == \"\") ");
			if (-1 == print_end(f, op, opt, op->u.fetch.end_bits, ir)) {
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

	dfavm_opasm_finalize_op(&a);

	return 0;
}

static int
fsm_print_awk_complete(FILE *f, const struct ir *ir,
	const struct fsm_options *opt, const char *prefix, const char *cp)
{
	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

	if (opt->fragment) {
		if (-1 == fsm_print_awkfrag(f, ir, opt, cp, prefix,
			opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque))
		{
			return -1;
		}
	} else {
		fprintf(f, "\n");

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

		if (-1 == fsm_print_awkfrag(f, ir, opt, cp, prefix,
			opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque))
		{
			return -1;
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
fsm_print_awk(FILE *f, const struct fsm *fsm)
{
	struct ir *ir;
	const char *prefix;
	const char *cp;
	int r;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return -1;
	}

	if (fsm->opt->prefix != NULL) {
		prefix = fsm->opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (fsm->opt->cp != NULL) {
		cp = fsm->opt->cp;
	} else {
		cp = "c"; /* XXX */
	}

	r = fsm_print_awk_complete(f, ir, fsm->opt, prefix, cp);

	free_ir(fsm, ir);

	return r;
}

