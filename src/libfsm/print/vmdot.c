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

#include "libfsm/vm/retlist.h"
#include "libfsm/vm/vm.h"

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
default_accept(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);

	(void) lang_opaque;
	(void) hook_opaque;
 
	fprintf(f, "match");

	if (state_metadata->end_id_count > 0) {
		fprintf(f, " / ");

		for (size_t i = 0; i < state_metadata->end_id_count; i++) {
			fprintf(f, "#%u", state_metadata->end_ids[i]);

			if (i < state_metadata->end_id_count - 1) {
				fprintf(f, " ");
			}
		}
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

	fprintf(f, "fail");

	return 0;
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
print_cond(FILE *f, const struct fsm_options *opt, const struct dfavm_op_ir *op)
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
print_end(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct dfavm_op_ir *op, enum dfavm_op_end end_bits)
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

		/* no print_hook_comment() for dot output */

		return 0;

	default:
		assert(!"unreached");
		abort();
	}
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "branch #%" PRIu32, op->u.br.dest_arg->index);
}

static int
fsm_print_nodes(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	struct dfavm_op_ir *ops)
{
	struct dfavm_op_ir *op;
	const char *example;
	unsigned block;

	assert(f != NULL);
	assert(opt != NULL);

	block = 0;

	example = NULL;

	for (op = ops; op != NULL; op = op->next) {
		if (op->num_incoming > 0 || op == ops) {
			if (op != ops) {
				fprintf(f, "\t\t</table>\n");
				fprintf(f, "\t> ];\n");
			}

			fprintf(f, "\n");

			fprintf(f, "\tS%" PRIu32 " [ label=<\n", op->index);
			fprintf(f, "\t\t<table border='0' cellborder='1' cellpadding='6' cellspacing='0'>\n");

			fprintf(f, "\t\t<tr>\n");
			if (op == ops) {
				fprintf(f, "\t\t\t<td><b>L-</b></td>\n");
			} else {
				fprintf(f, "\t\t\t<td><b>L%u</b></td>\n", block++);
			}
			fprintf(f, "\t\t\t<td><b>Cond</b></td>\n");
			fprintf(f, "\t\t\t<td align='left'><b>Op</b></td>\n");
			fprintf(f, "\t\t</tr>\n");
		}

		if (example != op->example) {
			if (op->example != NULL) {
				fprintf(f, "\t\t<tr>\n");
				fprintf(f, "\t\t\t<td colspan='3' align='left'>");
				fprintf(f, "e.g. <font face='mono'>\"");
				escputs(f, opt, dot_escputc_html, op->example);
				fprintf(f, "\"</font>");
				fprintf(f, "</td>\n");
				fprintf(f, "\t\t</tr>\n");
			}

			example = op->example;
		}

		fprintf(f, "\t\t<tr>\n");

		fprintf(f, "\t\t\t<td align='right' port='i%" PRIu32 "'>", op->index);
		fprintf(f, "#%" PRIu32, op->index);
		fprintf(f, "</td>\n");

		switch (op->instr) {
		case VM_OP_STOP:
			fprintf(f, "\t\t\t<td>");
			print_cond(f, opt, op);
			fprintf(f, "</td>\n");
			fprintf(f, "\t\t\t<td align='left' port='b%u'>", op->index);
			if (-1 == print_end(f, opt, hooks, op, op->u.stop.end_bits)) {
				return -1;
			}
			fprintf(f, "</td>\n");
			break;

		case VM_OP_FETCH:
			fprintf(f, "\t\t\t<td>fetch</td>\n");
			fprintf(f, "\t\t\t<td align='left' port='b%u'>", op->index);
			if (-1 == print_end(f, opt, hooks, op, op->u.fetch.end_bits)) {
				return -1;
			}
			fprintf(f, "</td>\n");
			break;

		case VM_OP_BRANCH:
			fprintf(f, "\t\t\t<td>");
			print_cond(f, opt, op);
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
fsm_print_edges(FILE *f, const struct fsm_options *opt, const struct dfavm_op_ir *ops)
{
	const struct dfavm_op_ir *op;
	unsigned long block;
	int can_fallthrough;

	assert(f != NULL);
	assert(opt != NULL);

	(void) opt;

	can_fallthrough = 1;

	for (op = ops; op != NULL; op = op->next) {
		if (op->num_incoming > 0 || op == ops) {
			if (op != ops && can_fallthrough) {
				fprintf(f, "\t");
				fprintf(f, "S%lu:s -> S%" PRIu32 ":n [ style = bold ];",
					block,
					op->index);
				if (opt->comments) {
					fprintf(f, " /* fallthrough */");
				}
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
			fprintf(f, "S%lu:b%" PRIu32 ":e -> S%lu:b%" PRIu32 ":e [ constraint = false ];",
				block,
				op->index,
				block,
				op->u.br.dest_arg->index);
			if (opt->comments) {
				fprintf(f, " /* relative */");
			}
		}

		fprintf(f, "\n");
	}
}

/* TODO: eventually to be non-static */
static int
fsm_print_vmdotfrag(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(retlist != NULL);

	if (-1 == fsm_print_nodes(f, opt, hooks, ops)) {
		return -1;
	}
	fprintf(f, "\n");

	fsm_print_edges(f, opt, ops);

	return 0;
}

int
fsm_print_vmdot(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(retlist != NULL);

	if (opt->fragment) {
		if (-1 == fsm_print_vmdotfrag(f, opt, hooks, retlist, ops)) {
			return -1;
		}
	} else {
		fprintf(f, "# generated\n");
		fprintf(f, "digraph G {\n");

		fprintf(f, "\tnode [ shape = plaintext ];\n");
		fprintf(f, "\trankdir = TB;\n");
		fprintf(f, "\tnodesep = 0.5;\n");
		fprintf(f, "\troot = S0;\n");

		fprintf(f, "\t{ start; S0; rank= same }\n");

		fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");
		fprintf(f, "\tstart -> S0:i0:w [ style = bold ];\n");

		if (-1 == fsm_print_vmdotfrag(f, opt, hooks, retlist, ops)) {
			return -1;
		}

		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	return 0;
}

