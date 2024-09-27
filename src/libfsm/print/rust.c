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
print_ids(FILE *f,
	enum fsm_ambig ambig, const fsm_end_id_t *ids, size_t count,
	size_t i)
{
	switch (ambig) {
	case AMBIG_NONE:
		fprintf(f, "return Some(())");
		break;

	case AMBIG_ERROR:
// TODO: decide if we deal with this ahead of the call to print or not
		if (count > 1) {
			errno = EINVAL;
			return -1;
		}

		fprintf(f, "return Some(%u)", ids[0]);
		break;

	case AMBIG_EARLIEST:
		/*
		 * The libfsm api guarentees these ids are unique,
		 * and only appear once each, and are sorted.
		 */
		fprintf(f, "return Some(%u)", ids[0]);
		break;

	case AMBIG_MULTIPLE:
		fprintf(f, "return Some(&RET%zu)", i);
		break;

	default:
		assert(!"unreached");
		abort();
	}

	return 0;
}

static int
default_accept(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	size_t i;

	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque != NULL);

	(void) hook_opaque;

	i = * (const size_t *) lang_opaque;

	if (-1 == print_ids(f, opt->ambig, state_metadata->end_ids, state_metadata->end_id_count, i)) {
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

	fprintf(f, "return None");

	return 0;
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
print_end(FILE *f, const struct dfavm_op_ir *op,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	enum dfavm_op_end end_bits)
{
	size_t i;

	switch (end_bits) {
	case VM_END_FAIL:
		return print_hook_reject(f, opt, hooks, default_reject, NULL);

	case VM_END_SUCC:
		assert(op->ret >= retlist->a);

		i = op->ret - retlist->a;

		const struct fsm_state_metadata state_metadata = {
			.end_ids = op->ret->ids,
			.end_id_count = op->ret->count,
		};

		return print_hook_accept(f, opt, hooks,
			&state_metadata, default_accept,
			&i);

	default:
		assert(!"unreached");
		abort();
	}
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

static void
print_ret(FILE *f, const unsigned *ids, size_t count)
{
	size_t i;

	fprintf(f, "[");
	for (i = 0; i < count; i++) {
		fprintf(f, "%u", ids[i]);
		if (i + 1 < count) {
			fprintf(f, ", ");
		}
	}
	fprintf(f, "];");
}

/* TODO: eventually to be non-static */
static int
fsm_print_rustfrag(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops,
	const char *cp)
{
	struct dfavm_op_ir *op;
	bool fallthrough;

	assert(f != NULL);
	assert(opt != NULL);
	assert(retlist != NULL);
	assert(cp != NULL);

	/* TODO: we'll need to heed cp for e.g. lx's codegen */
	(void) cp;

	if (opt->ambig == AMBIG_MULTIPLE) {
		for (size_t i = 0; i < retlist->count; i++) {
			fprintf(f, "    static RET%zu: [u32; %zu] = ", i, retlist->a[i].count);
			print_ret(f, retlist->a[i].ids, retlist->a[i].count);
			fprintf(f, "\n");
		}
		fprintf(f, "\n");
	}

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

			if (op->example != NULL) {
				/* C's escaping seems to be a subset of rust's, and these are
				 * for comments anyway. So I'm borrowing this for C here */
				fprintf(f, " // e.g. \"");
				escputs(f, opt, c_escputc_str, op->example);
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
			if (-1 == print_end(f, op, opt, hooks, retlist, op->u.stop.end_bits)) {
				return -1;
			}
			if (op->cmp != VM_CMP_ALWAYS) {
				fprintf(f, " }");
			}

			if (op->u.stop.end_bits == VM_END_SUCC) {
				const struct fsm_state_metadata state_metadata = {
					.end_ids = op->ret->ids,
					.end_id_count = op->ret->count,
				};
				if (-1 == print_hook_comment(f, opt, hooks,
					&state_metadata))
				{
					return -1;
				}
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
				print_end(f, op, opt, hooks, retlist, op->u.fetch.end_bits);
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
fsm_print_rust(FILE *f,
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

	fprintf(f, "// generated\n");

	if (opt->fragment) {
		fsm_print_rustfrag(f, opt, hooks, retlist, ops, cp);
		goto error;
	}

	fprintf(f, "\n");

	fprintf(f, "pub fn %smain", prefix);

	switch (opt->io) {
	case FSM_IO_GETC:
		/* e.g. dbg!(fsm_main("abc".as_bytes().iter().copied())); */
		fprintf(f, "(mut bytes: impl Iterator<Item = u8>)");
		break;

	case FSM_IO_STR:
		/* e.g. dbg!(fsm_main("xabces")); */
		fprintf(f, "(%sinput: &str)",
			has_op(ops, VM_OP_FETCH) ? "" : "_");
		break;

	case FSM_IO_PAIR:
		/* e.g. dbg!(fsm_main("xabces".as_bytes())); */
		fprintf(f, "(%sinput: &[u8])",
			has_op(ops, VM_OP_FETCH) ? "" : "_");
		break;

	default:
		fprintf(stderr, "unsupported IO API\n");
		exit(EXIT_FAILURE);
	}

	fprintf(f, " -> ");

	switch (opt->ambig) {
	case AMBIG_NONE:
		fprintf(f, "Option<()>");
		break;

	case AMBIG_ERROR:
	case AMBIG_EARLIEST:
		fprintf(f, "Option<u32>");
		break;

	case AMBIG_MULTIPLE:
		fprintf(f, "Option<&'static [u32]>");
		break;

	default:
		fprintf(stderr, "unsupported ambig mode\n");
		exit(EXIT_FAILURE);
	}

	fprintf(f, " {\n");
	fprintf(f, "    use Label::*;\n");

	fsm_print_rustfrag(f, opt, hooks, retlist, ops, cp);

	fprintf(f, "}\n");
	fprintf(f, "\n");

	return 0;

error:

	return -1;
}

