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
leaf(FILE *f, const struct fsm_end_ids *ids, const void *leaf_opaque)
{
	assert(f != NULL);
	assert(leaf_opaque == NULL);

	(void) ids;
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
		assert("unreached");
		return NULL;
	}
}

static void
print_label(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	fprintf(f, "l%lu:", (unsigned long) op->index);

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

static void
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
	enum dfavm_op_end end_bits, const struct ir *ir)
{
	if (end_bits == VM_END_FAIL) {
		fprintf(f, "{\n\t\treturn -1\n\t}\n");
		return;
	}

	if (opt->endleaf != NULL) {
		opt->endleaf(f, op->ir_state->end_ids, opt->endleaf_opaque);
	} else {
		fprintf(f, "{\n\t\treturn %lu\n\t}\n", (unsigned long) (op->ir_state - ir->states));
	}
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "{\n\t\tgoto l%lu\n\t}\n", (unsigned long) op->u.br.dest_arg->index);
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
fsm_print_gofrag(FILE *f, const struct ir *ir, const struct fsm_options *opt,
	const char *cp,
	int (*leaf)(FILE *, const struct fsm_end_ids *ids, const void *leaf_opaque),
	const void *leaf_opaque)
{
	static const struct dfavm_assembler_ir zero;
	struct dfavm_assembler_ir a;
	struct dfavm_op_ir *op;

	static const struct fsm_vm_compile_opts vm_opts = { FSM_VM_COMPILE_DEFAULT_FLAGS, FSM_VM_COMPILE_VM_V1, NULL };

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);
	assert(cp != NULL);

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

		l = 0;

		for (op = a.linked; op != NULL; op = op->next) {
			if (op->num_incoming > 0) {
				op->index = l++;
			}
		}
	}

	for (op = a.linked; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			print_label(f, op, opt);
			fprintf(f, "\n");
		}

		fprintf(f, "\t");

		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op, opt);
			print_end(f, op, opt, op->u.stop.end_bits, ir);
			break;

		case VM_OP_FETCH:
			print_fetch(f, opt);
			print_end(f, op, opt, op->u.fetch.end_bits, ir);
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

	dfavm_opasm_finalize_op(&a);

	return 0;
}

static void
fsm_print_go_complete(FILE *f, const struct ir *ir, const struct fsm_options *opt)
{
	/* TODO: currently unused, but must be non-NULL */
	const char *cp = "";

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

	(void) fsm_print_gofrag(f, ir, opt, cp,
		opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque);
}

void
fsm_print_go(FILE *f, const struct fsm *fsm)
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
		fsm_print_go_complete(f, ir, fsm->opt);
		return;
	}

	fprintf(f, "package %sfsm\n", prefix);
	fprintf(f, "\n");

	fprintf(f, "func Match");

	switch (fsm->opt->io) {
	case FSM_IO_PAIR:
		fprintf(f, "(data []byte) int {\n");
		if (ir->n > 0) {
			/* start idx at -1 unsigned so after first increment we're correct at index 0 */
			fprintf(f, "\tvar idx = ^uint(0)\n");
			fprintf(f, "\n");
		}
		break;

	case FSM_IO_STR:
		fprintf(f, "(data string) int {\n");
		if (ir->n > 0) {
			/* start idx at -1 unsigned so after first increment we're correct at index 0 */
			fprintf(f, "\tvar idx = ^uint(0)\n");
			fprintf(f, "\n");
		}
		break;

	default:
		fprintf(stderr, "unsupported IO API\n");
		exit(EXIT_FAILURE);
	}

	fsm_print_go_complete(f, ir, fsm->opt);

	fprintf(f, "}\n");

	free_ir(fsm, ir);
}

