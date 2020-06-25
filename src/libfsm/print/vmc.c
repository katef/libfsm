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
	fprintf(f, "l%lu:", (unsigned long) op->index);
}

static void
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	if (op->cmp == VM_CMP_ALWAYS) {
		return;
	}

	fprintf(f, "if (c %s ", cmp_operator(op->cmp));
	c_escputcharlit(f, opt, op->cmp_arg);
	fprintf(f, ") ");
}

static void
print_stop(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "goto %s;", op->u.stop.end_bits == VM_END_FAIL ? "fail" : "match");
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "goto l%lu;", (unsigned long) op->u.br.dest_arg->index);
}

static void
print_fetch(FILE *f, const struct fsm_options *opt)
{
	switch (opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "if (c = fsm_getc(opaque), c == EOF) ");
		break;

	case FSM_IO_STR:
		fprintf(f, "if (c = *p++, c == '\\0') ");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "if (c = *p++, p == e) ");
		break;
	}
}

/* TODO: eventually to be non-static */
static int
fsm_print_cfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt,
	const char *cp,
	int (*leaf)(FILE *, const void *state_opaque, const void *leaf_opaque),
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
			print_label(f, op);
			fprintf(f, "\n");
		}

		fprintf(f, "\t");

		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op, opt);
			print_stop(f, op);
			break;

		case VM_OP_FETCH:
			print_fetch(f, opt);
			print_stop(f, op);
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
fsm_print_c_complete(FILE *f, const struct ir *ir, const struct fsm_options *opt)
{
	const char *cp;

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

	(void) fsm_print_cfrag(f, ir, opt, cp,
		opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque);

	fprintf(f, "\n");

	fprintf(f, "match:\n");
	fprintf(f, "\treturn 0x1;\n"); /* TODO: eventually switch on state here */

	fprintf(f, "fail:\n");
	fprintf(f, "\treturn -1;\n");
}

void
fsm_print_vmc(FILE *f, const struct fsm *fsm)
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

	fprintf(f, "\n");

	fprintf(f, "int\n%smain", prefix);

	switch (fsm->opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "(int (*fsm_getc)(void *opaque), void *opaque)\n");
		fprintf(f, "{\n");
		fprintf(f, "\tint c;\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "(const char *s)\n");
		fprintf(f, "{\n");
		fprintf(f, "\tconst char *p;\n");
		fprintf(f, "\tint c;\n");
		fprintf(f, "\n");
		fprintf(f, "\tp = s;\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "(const char *b, const char *e)\n");
		fprintf(f, "{\n");
		fprintf(f, "\tconst char *p;\n");
		fprintf(f, "\tint c;\n");
		fprintf(f, "\n");
		fprintf(f, "\tp = b;\n");
		fprintf(f, "\n");
		break;
	}

	fsm_print_c_complete(f, ir, fsm->opt);

	fprintf(f, "}\n");
	fprintf(f, "\n");

	free_ir(fsm, ir);
}

