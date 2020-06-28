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

static const char *
cmp_operator(int cmp)
{
	switch (cmp) {
	case VM_CMP_LT: return "-lt";
	case VM_CMP_LE: return "-le";
	case VM_CMP_EQ: return "-eq";
	case VM_CMP_GE: return "-ge";
	case VM_CMP_GT: return "-gt";
	case VM_CMP_NE: return "-ne";

	case VM_CMP_ALWAYS:
	default:
		assert("unreached");
		return NULL;
	}
}

static void
print_label(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	fprintf(f, "l%lu)", (unsigned long) op->index);

	/*
	 * The example strings here are just for human-readable comments;
	 * double quoted strings in sh do not support numeric escapes for
	 * arbitrary characters, and nobody can read $'...' style strings.
	 * So I'm just using C-style strings here because it's simple enough.
	 */
	if (op->ir_state->example != NULL) {
		fprintf(f, " # e.g. \"");
		escputs(f, opt, c_escputc_str, op->ir_state->example);
		fprintf(f, "\"");
	}
}

static void
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	(void) opt;

	if (op->cmp == VM_CMP_ALWAYS) {
		return;
	}

	fprintf(f, "[ $(ord \"$c\") %s ", cmp_operator(op->cmp));
	fprintf(f, "%hhu", (unsigned char) op->cmp_arg);
	fprintf(f, " ] && ");
}

static void
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
	enum dfavm_op_end end_bits, const struct ir *ir)
{
	if (end_bits == VM_END_FAIL) {
		fprintf(f, "fail");
		return;
	}

	if (opt->endleaf != NULL) {
		opt->endleaf(f, op->ir_state->opaque, opt->endleaf_opaque);
	} else {
		fprintf(f, "matched %lu", (unsigned long) (op->ir_state - ir->states));
	}
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "{ l=l%lu; continue; }", (unsigned long) op->u.br.dest_arg->index);
}

static void
print_fetch(FILE *f, const struct fsm_options *opt)
{
	(void) opt;

	fprintf(f, "read -n 1 c || ");
}

static int
fsm_print_shfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt)
{
	static const struct dfavm_assembler_ir zero;
	struct dfavm_assembler_ir a;
	struct dfavm_op_ir *op;

	static const struct fsm_vm_compile_opts vm_opts = { FSM_VM_COMPILE_DEFAULT_FLAGS, FSM_VM_COMPILE_VM_V1, NULL };

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

	a = zero;

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

	fprintf(f, "ord() { printf %%d \"'$1\"; }\n");
	fprintf(f, "fail() { echo fail; exit 1; }\n");
	fprintf(f, "matched() { echo match; exit 0; }\n");
	fprintf(f, "\n");

	fprintf(f, "l=start\n");
	fprintf(f, "\n");

	fprintf(f, "while true; do\n");
	fprintf(f, "\tcase $l in\n");
	fprintf(f, "\tstart)\n");

	for (op = a.linked; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			if (op != a.linked) {
				fprintf(f, "\t\t;&\n");
			}

			fprintf(f, "\t");
			print_label(f, op, opt);
			fprintf(f, "\n");
		}

		fprintf(f, "\t\t");

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

	fprintf(f, "\t\t;;\n");
	fprintf(f, "\tesac\n");
	fprintf(f, "done\n");

	dfavm_opasm_finalize_op(&a);

	return 0;
}

void
fsm_print_sh(FILE *f, const struct fsm *fsm)
{
	struct ir *ir;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return;
	}

	if (fsm->opt->io != FSM_IO_STR) {
		fprintf(stderr, "unsupported IO API\n");
		exit(EXIT_FAILURE);
	}

	(void) fsm_print_shfrag(f, ir, fsm->opt);

	free_ir(fsm, ir);
}

