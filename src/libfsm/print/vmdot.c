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

static unsigned int
ir_indexof(const struct ir *ir, const struct ir_state *cs)
{
	assert(ir != NULL);
	assert(cs != NULL);

	return cs - &ir->states[0];
}

static const char *
cmp_operator(int cmp)
{
	switch (cmp) {
	case VM_CMP_LT: return "&lt;";
	case VM_CMP_LE: return "&le;";
	case VM_CMP_EQ: return "=";
	case VM_CMP_GE: return "&ge;";
	case VM_CMP_GT: return "&gt;";
	case VM_CMP_NE: return "&ne;";

	case VM_CMP_ALWAYS:
	default:
		assert("unreached");
		return NULL;
	}
}

static int
op_can_fallthrough(const struct dfavm_op_ir *op)
{
	if (op->instr == VM_OP_STOP && op->cmp == VM_CMP_ALWAYS) {
		return 0;
	}

	if (op->instr == VM_OP_BRANCH && op->cmp == VM_CMP_ALWAYS) {
		return 0;
	}

	return 1;
}

static void
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	if (op->cmp == VM_CMP_ALWAYS) {
		fprintf(f, "always");
		return;
	}

	fprintf(f, "%s '", cmp_operator(op->cmp));
	dot_escputc_html(f, opt, op->cmp_arg);
	fprintf(f, "'");
}

static int
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
	enum dfavm_op_end end_bits, const struct ir *ir)
{
	if (end_bits == VM_END_FAIL) {
		fprintf(f, "fail");
		return 0;
	}

	if (opt->endleaf != NULL) {
		if (-1 == opt->endleaf(f, op->ir_state->end_ids, opt->endleaf_opaque)) {
			return -1;
		}
	} else {
		fprintf(f, "ret %td", op->ir_state - ir->states);
	}

	return 0;
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "branch #%" PRIu32, op->u.br.dest_arg->index);
}

static int
fsm_print_nodes(FILE *f, const struct ir *ir, const struct fsm_options *opt,
	const struct dfavm_assembler_ir *a)
{
	const struct dfavm_op_ir *op;
	const struct ir_state *ir_state;
	unsigned block;

	block = 0;

	ir_state = NULL;

	for (op = a->linked; op != NULL; op = op->next) {
		if (op->num_incoming > 0 || op == a->linked) {
			if (op != a->linked) {
				fprintf(f, "\t\t</table>\n");
				fprintf(f, "\t> ];\n");
			}

			fprintf(f, "\n");

			fprintf(f, "\tS%" PRIu32 " [ label=<\n", op->index);
			fprintf(f, "\t\t<table border='0' cellborder='1' cellpadding='6' cellspacing='0'>\n");

			fprintf(f, "\t\t<tr>\n");
			if (op == a->linked) {
				fprintf(f, "\t\t\t<td><b>L-</b></td>\n");
			} else {
				fprintf(f, "\t\t\t<td><b>L%u</b></td>\n", block++);
			}
			fprintf(f, "\t\t\t<td><b>State</b></td>\n");
			fprintf(f, "\t\t\t<td><b>Cond</b></td>\n");
			fprintf(f, "\t\t\t<td align='left'><b>Op</b></td>\n");
			fprintf(f, "\t\t</tr>\n");
		}

		if (ir_state != op->ir_state || op->num_incoming > 0) {
			if (op->ir_state->example != NULL) {
				fprintf(f, "\t\t<tr>\n");
				fprintf(f, "\t\t\t<td colspan='4' align='left'>");
				fprintf(f, "e.g. <font face='mono'>\"");
				escputs(f, opt, dot_escputc_html, op->ir_state->example);
				fprintf(f, "\"</font>");
				fprintf(f, "</td>\n");
				fprintf(f, "\t\t</tr>\n");
			}

			ir_state = op->ir_state;
		}

		fprintf(f, "\t\t<tr>\n");

		fprintf(f, "\t\t\t<td align='right' port='i%" PRIu32 "'>", op->index);
		fprintf(f, "#%" PRIu32, op->index);
		fprintf(f, "</td>\n");

		fprintf(f, "\t\t\t<td>");
		if (op->ir_state != NULL) {
			fprintf(f, op->ir_state->isend ? "(S%u)" : "S%u", ir_indexof(ir, op->ir_state));
		} else {
			fprintf(f, "(none)");
		}
		fprintf(f, "</td>\n");

		switch (op->instr) {
		case VM_OP_STOP:
			fprintf(f, "\t\t\t<td>");
			print_cond(f, op, opt);
			fprintf(f, "</td>\n");
			fprintf(f, "\t\t\t<td align='left' port='b%u'>", op->index);
			if (-1 == print_end(f, op, opt, op->u.stop.end_bits, ir)) {
				return -1;
			}
			fprintf(f, "</td>\n");
			break;

		case VM_OP_FETCH:
			fprintf(f, "\t\t\t<td>fetch</td>\n");
			fprintf(f, "\t\t\t<td align='left' port='b%u'>", op->index);
			if (-1 == print_end(f, op, opt, op->u.fetch.end_bits, ir)) {
				return -1;
			}
			fprintf(f, "</td>\n");
			break;

		case VM_OP_BRANCH:
			fprintf(f, "\t\t\t<td>");
			print_cond(f, op, opt);
			fprintf(f, "</td>\n");
			fprintf(f, "\t\t\t<td align='left' port='b%u'>", op->index);
			print_branch(f, op);
			fprintf(f, "</td>\n");
			break;

		default:
			assert(!"unreached");
			break;
		}

		fprintf(f, "\t\t</tr>\n");
	}

	fprintf(f, "\t\t</table>\n");
	fprintf(f, "\t> ];\n");

	return 0;
}

static void
fsm_print_edges(FILE *f, const struct fsm_options *opt,
	const struct dfavm_assembler_ir *a)
{
	const struct dfavm_op_ir *op;
	unsigned long block;
	int can_fallthrough;

	(void) opt;

	can_fallthrough = 1;

	for (op = a->linked; op != NULL; op = op->next) {
		if (op->num_incoming > 0 || op == a->linked) {
			if (op != a->linked && can_fallthrough) {
				fprintf(f, "\t");
				fprintf(f, "S%lu:s -> S%" PRIu32 ":n [ style = bold ]; /* fallthrough */",
					block,
					op->index);
				fprintf(f, "\n");
			}

			block = op->index;
		}

		can_fallthrough = op_can_fallthrough(op);

		if (op->instr != VM_OP_BRANCH) {
			continue;
		}

		fprintf(f, "\t");

		/*
		 * The instructions have been sorted by the VM optimisation somewhat,
		 * similar to a topological sort. We're taking advantage of that here
		 * by placing backward edges on the left, forward edges on the right.
		 * This helps give some visual consistency to the layout.
		 */
		if (op->u.br.dest_arg->index > block) {
			/* branch to a later block */
			fprintf(f, "S%lu:b%" PRIu32 ":e -> S%" PRIu32 ";",
				block,
				op->index,
				op->u.br.dest_arg->index);
		} else if (op->u.br.dest_arg->index < block) {
			/* branch to an earlier block */
			fprintf(f, "S%lu:i%" PRIu32 ":w -> S%" PRIu32 ";",
				block,
				op->index,
				op->u.br.dest_arg->index);
		} else {
			/* relative branch within the same block, entry on the east */
			/* XXX: would like to make these edges shorter, but I don't know how */
			fprintf(f, "S%lu:b%" PRIu32 ":e -> S%lu:b%" PRIu32 ":e [ constraint = false ]; /* relative */",
				block,
				op->index,
				block,
				op->u.br.dest_arg->index);
		}

		fprintf(f, "\n");
	}
}

/* TODO: eventually to be non-static */
static int
fsm_print_vmdotfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt)
{
	static const struct dfavm_assembler_ir zero;
	struct dfavm_assembler_ir a;

	static const struct fsm_vm_compile_opts vm_opts = { FSM_VM_COMPILE_DEFAULT_FLAGS, FSM_VM_COMPILE_VM_V1, NULL };

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

	a = zero;

	if (!dfavm_compile_ir(&a, ir, vm_opts)) {
		return -1;
	}

	if (-1 == fsm_print_nodes(f, ir, opt, &a)) {
		return -1;
	}
	fprintf(f, "\n");

	fsm_print_edges(f, opt, &a);

	dfavm_opasm_finalize_op(&a);

	return 0;
}


static int
fsm_print_vmdot_complete(FILE *f, const struct ir *ir, const struct fsm_options *opt)
{
	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

	if (opt->fragment) {
		if (-1 == fsm_print_vmdotfrag(f, ir, opt)) {
			return -1;
		}
	} else {
		fprintf(f, "\n");

		fprintf(f, "digraph G {\n");

		fprintf(f, "\tnode [ shape = plaintext ];\n");
		fprintf(f, "\trankdir = TB;\n");
		fprintf(f, "\tnodesep = 0.5;\n");
		fprintf(f, "\troot = S0;\n");

		fprintf(f, "\t{ start; S0; rank= same }\n");

		fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");
		fprintf(f, "\tstart -> S0:i0:w [ style = bold ];\n");

		if (-1 == fsm_print_vmdotfrag(f, ir, opt)) {
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
fsm_print_vmdot(FILE *f, const struct fsm *fsm)
{
	struct ir *ir;
	int r;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return -1;
	}

	/* henceforth, no function should be passed struct fsm *, only the ir and options */

	r = fsm_print_vmdot_complete(f, ir, fsm->opt);

	free_ir(fsm, ir);

	return r;
}

