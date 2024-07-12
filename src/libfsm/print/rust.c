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
print_leaf(FILE *f, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque)
{
	assert(f != NULL);
	assert(leaf_opaque == NULL);

	(void) ids;
	(void) count;
	(void) leaf_opaque;

	fprintf(f, "return None;");

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

static int
has_op(const struct dfavm_op_ir *ops, enum dfavm_op_instr instr)
{
	const struct dfavm_op_ir *op;

	for (op = ops; op != NULL; op = op->next) {
		if (op->instr == instr) {
			return 1;
		}
	}

	return 0;
}

static void
print_label(FILE *f, const struct dfavm_op_ir *op)
{
	if (op->index == START) {
		fprintf(f, "Ls");
	} else {
		fprintf(f, "L%" PRIu32, op->index);
	}
}

static void
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	if (op->cmp == VM_CMP_ALWAYS) {
		return;
	}

	fprintf(f, "if c %s ", cmp_operator(op->cmp));
	rust_escputcharlit(f, opt, op->cmp_arg);
	fprintf(f, " ");
}

static int
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
	const struct ir_state *ir_states,
	enum dfavm_op_end end_bits)
{
	if (end_bits == VM_END_FAIL) {
		fprintf(f, "return None");
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
		fprintf(f, "return Some(())");
	}

	return 0;
}

static void
print_jump(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "l = ");
	print_label(f, op);
	fprintf(f, "; continue;");
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
	print_jump(f, op->u.br.dest_arg);
}

static void
print_fetch(FILE *f)
{
	fprintf(f, "bytes.next()");
}

/* TODO: eventually to be non-static */
static int
fsm_print_rustfrag(FILE *f, const struct fsm_options *opt,
	const struct ir *ir, struct dfavm_op_ir *ops,
	const char *cp,
	int (*leaf)(FILE *, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque),
	const void *leaf_opaque)
{
	struct dfavm_op_ir *op;
	bool fallthrough;

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

		l = START;

		for (op = ops; op != NULL; op = op->next) {
			if (op == ops || op->num_incoming > 0) {
				op->index = l++;
			}
		}
	}

	/*
	 * Only declare variables if we're actually going to use them.
	 */
	if (ops->cmp == VM_CMP_ALWAYS && ops->instr == VM_OP_STOP) {
		assert(ops->next == NULL);
		fprintf(f, "\n");
	} else {
		switch (opt->io) {
		case FSM_IO_GETC:
			break;

		case FSM_IO_STR:
			fprintf(f, "    let mut bytes = input.bytes();\n");
			fprintf(f, "\n");
			break;

		case FSM_IO_PAIR:
			fprintf(f, "    let mut bytes = input.iter();\n");
			fprintf(f, "\n");
			break;

		default:
			assert(!"unreached");
			break;
		}
	}

	fprintf(f, "    pub enum Label {\n       ");
	for (op = ops; op != NULL; op = op->next) {
		if (op == ops || op->num_incoming > 0) {
			fprintf(f, " ");
			print_label(f, op);
			fprintf(f, ",");
		}
	}
	fprintf(f, "\n");
	fprintf(f, "    }\n");
	fprintf(f, "\n");

	fprintf(f, "    let %sl = Ls;\n", has_op(ops, VM_OP_BRANCH) ? "mut " : "");
	fprintf(f, "\n");

	fprintf(f, "    loop {\n");
	fprintf(f, "        match l {\n");

	fallthrough = true;

	for (op = ops; op != NULL; op = op->next) {
		if (op == ops || op->num_incoming > 0) {
			if (op != ops) {
				if (fallthrough) {
					fprintf(f, "                ");
					print_jump(f, op);
					fprintf(f, "\n");
				}
				fprintf(f, "            }\n");
				fprintf(f, "\n");
			}

			fprintf(f, "            ");
			print_label(f, op);
			fprintf(f, " => {");

			if (op->ir_state != NULL && op->ir_state->example != NULL) {
				/* C's escaping seems to be a subset of rust's, and these are
				 * for comments anyway. So I'm borrowing this for C here */
				fprintf(f, " // e.g. \"");
				escputs(f, opt, c_escputc_str, op->ir_state->example);
				fprintf(f, "\"");
			}

			fprintf(f, "\n");
		}

		fallthrough = true;

		fprintf(f, "                ");

		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op, opt);
			if (op->cmp != VM_CMP_ALWAYS) {
				fprintf(f, "{ ");
			}
			if (-1 == print_end(f, op, opt, ir->states, op->u.stop.end_bits)) {
				return -1;
			}
			fprintf(f, ";");
			if (op->cmp != VM_CMP_ALWAYS) {
				fprintf(f, " }");
			}

			if (op->cmp == VM_CMP_ALWAYS) {
				/* the code for fallthrough would be unreachable */
				fallthrough = false;
			}
			break;

		case VM_OP_FETCH: {
			const char *c, *ref;

			/*
			 * If the following instruction is an unconditional return,
			 * we won't be using this value.
			 */
			if (op->next != NULL && op->next->instr == VM_OP_STOP && op->next->cmp == VM_CMP_ALWAYS) {
				c =  "_c";
			} else {
				c = "c";
			}

			switch (opt->io) {
			case FSM_IO_GETC: ref = "";  break;
			case FSM_IO_STR:  ref = "";  break;
			case FSM_IO_PAIR: ref = "&"; break;

			default:
				assert(!"unreached");
				break;
			}

			/* a more compact form, as an aesthetic optimisation */
			if (op->u.fetch.end_bits == VM_END_FAIL) {
				fprintf(f, "let %s%s = ", ref, c);
				print_fetch(f);
				fprintf(f, "?;");
			} else {
				fprintf(f, "let %s%s = match ", ref, c);
				print_fetch(f);
				fprintf(f, " {\n");

				fprintf(f, "                    ");
				fprintf(f, "None => ");
				print_end(f, op, opt, ir->states, op->u.fetch.end_bits);
				fprintf(f, ",\n");
				fprintf(f, "                    ");

				fprintf(f, "Some(%s) => %s", c, c);
				fprintf(f, ",\n");
				fprintf(f, "                ");
				fprintf(f, "};");
			}

			break;
		}

		case VM_OP_BRANCH:
			print_cond(f, op, opt);
			if (op->cmp != VM_CMP_ALWAYS) {
				fprintf(f, "{ ");
			}
			print_branch(f, op);
			if (op->cmp != VM_CMP_ALWAYS) {
				fprintf(f, " }");
			}

			if (op->cmp == VM_CMP_ALWAYS) {
				/* the code for fallthrough would be unreachable */
				fallthrough = false;
			}
			break;

		default:
			assert(!"unreached");
			break;
		}

		fprintf(f, "\n");
	}

	fprintf(f, "            }\n");
	fprintf(f, "        }\n");
	fprintf(f, "    }\n");

	return 0;
}

int
fsm_print_rust(FILE *f, const struct fsm_options *opt,
	const struct ir *ir, struct dfavm_op_ir *ops)
{
	int (*leaf)(FILE *f, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque);
	const char *prefix;
	const char *cp;

	assert(f != NULL);
	assert(opt != NULL);

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

	if (opt->cp != NULL) {
		cp = opt->cp;
	} else {
		cp = "c"; /* XXX */
	}

	if (opt->fragment) {
		fsm_print_rustfrag(f, opt, ir, ops, cp,
			leaf, opt->leaf_opaque);
		goto error;
	}

	fprintf(f, "\n");

	fprintf(f, "pub fn %smain", prefix);

	switch (opt->io) {
	case FSM_IO_GETC:
		/* e.g. dbg!(fsm_main("abc".as_bytes().iter().copied())); */
		fprintf(f, "(mut bytes: impl Iterator<Item = u8>) -> Option<()> {\n");
		fprintf(f, "    use Label::*;\n");
		break;

	case FSM_IO_STR:
		/* e.g. dbg!(fsm_main("xabces")); */
		fprintf(f, "(%sinput: &str) -> Option<()> {\n",
			has_op(ops, VM_OP_FETCH) ? "" : "_");
		fprintf(f, "    use Label::*;\n");
		break;

	case FSM_IO_PAIR:
		/* e.g. dbg!(fsm_main("xabces".as_bytes())); */
		fprintf(f, "(%sinput: &[u8]) -> Option<()> {\n",
			has_op(ops, VM_OP_FETCH) ? "" : "_");
		fprintf(f, "    use Label::*;\n");
		break;

	default:
		fprintf(stderr, "unsupported IO API\n");
		exit(EXIT_FAILURE);
	}

	fsm_print_rustfrag(f, opt, ir, ops, cp,
		leaf, opt->leaf_opaque);

	fprintf(f, "}\n");
	fprintf(f, "\n");

	return 0;

error:

	return -1;
}

