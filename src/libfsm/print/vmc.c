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

// TODO: centralise vmc/c
static int
print_ids(FILE *f,
	enum fsm_ambig ambig, const struct fsm_state_metadata *state_metadata)
{
	switch (ambig) {
	case AMBIG_NONE:
		// TODO: for C99 we'd return bool
		fprintf(f, "return 1;");
		break;

	case AMBIG_ERROR:
// TODO: decide if we deal with this ahead of the call to print or not
		if (state_metadata->end_id_count > 1) {
			errno = EINVAL;
			return -1;
		}

		/* fallthrough */

	case AMBIG_EARLIEST:
		/*
		 * The libfsm api guarentees these ids are unique,
		 * and only appear once each, and are sorted.
		 */
		fprintf(f, "{\n");
		fprintf(f, "\t\t*id = %u;\n", state_metadata->end_ids[0]);
		fprintf(f, "\t\treturn 1;\n");
		fprintf(f, "\t}");
		break;

	case AMBIG_MULTIPLE:
		/*
		 * Here I would like to emit (static unsigned []) { 1, 2, 3 }
		 * but specifying a storage duration for compound literals
		 * is a compiler extension.
		 * So I'm emitting a static const variable declaration instead.
		 */

		fprintf(f, "{\n");
		fprintf(f, "\t\tstatic const unsigned a[] = { ");
		for (fsm_end_id_t i = 0; i < state_metadata->end_id_count; i++) {
			fprintf(f, "%u", state_metadata->end_ids[i]);
			if (i + 1 < state_metadata->end_id_count) {
				fprintf(f, ", ");
			}
		}
		fprintf(f, " };\n");
		fprintf(f, "\t\t*ids = a;\n");
		fprintf(f, "\t\t*count = %zu;\n", state_metadata->end_id_count);
		fprintf(f, "\t\treturn 1;\n");
		fprintf(f, "\t}");
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
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);

	(void) lang_opaque;
	(void) hook_opaque;

	if (-1 == print_ids(f, opt->ambig, state_metadata)) {
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

	fprintf(f, "return 0;");

	return 0;
}

static void
print_label(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	fprintf(f, "l%" PRIu32 ":", op->index);

	if (op->example != NULL) {
		fprintf(f, " /* e.g. \"");
		escputs(f, opt, c_escputc_str, op->example);
		fprintf(f, "\" */");
	}
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

static int
print_end(FILE *f, const struct dfavm_op_ir *op,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	enum dfavm_op_end end_bits)
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

		if (-1 == print_hook_comment(f, opt, hooks,
			&state_metadata))
		{
			return -1;
		}

		return 0;

	default:
		assert(!"unreached");
		abort();
	}
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "goto l%" PRIu32 ";", op->u.br.dest_arg->index);
}

static void
print_fetch(FILE *f, const struct fsm_options *opt)
{
	switch (opt->io) {
	case FSM_IO_GETC:
		/*
		 * Per its API, fsm_getc() is expected to return a positive character
		 * value (as if cast via unsigned char), or EOF. Just like fgetc() does.
		 */
		fprintf(f, "if (c = fsm_getc(getc_opaque), c == EOF) ");
		break;

	case FSM_IO_STR:
		fprintf(f, "if (c = (unsigned char) *p++, c == '\\0') ");
		break;

	case FSM_IO_PAIR:
		/* This is split into two parts.  The first part checks if we're
		 * at the end of the buffer.  The second part, in
		 * fsm_print_cfrag, fetches the byte
		 */
		fprintf(f, "if (p == e) ");
		break;
	}
}

static size_t
walk_sequence(struct dfavm_op_ir *op,
	enum dfavm_op_end *end_bits,
	char *buf, size_t len,
	struct dfavm_op_ir **tail)
{
	size_t n;

	assert(end_bits != NULL);
	assert(buf != NULL);
	assert(tail != NULL);

	/*
	 * Here we're looking for a sequence of:
	 *
	 *   FETCH: (or fail)
	 *   STOP: c != 'x' (or fail)
	 *
	 * This catches situations like the "abc" and "xyz" in /^abc[01]xyz/,
	 * but not for runs in unanchored regexes. For those, we'd be better
	 * off adding VM instructions for sets of strings, and producing
	 * those from the various types of enum ir_strategy.
	 */

	n = 0;

	/* fetch */
	{
		if (op == NULL || op->instr != VM_OP_FETCH) {
			goto unsuitable;
		}

		assert(op->cmp == VM_CMP_ALWAYS);

		/* op->num_incoming > 0 is allowed for this instruction only */

		if (op->u.fetch.end_bits != VM_END_FAIL) {
			goto unsuitable;
		}

		*end_bits = op->u.fetch.end_bits;
	}

	op = op->next;

	/* stop */
	{
		if (op == NULL || op->instr != VM_OP_STOP) {
			goto unsuitable;
		}

		if (op->cmp != VM_CMP_NE) {
			goto unsuitable;
		}

		if (op->num_incoming > 0) {
			goto unsuitable;
		}

		if (op->u.stop.end_bits != *end_bits) {
			goto unsuitable;
		}
	}

	if (n >= len) {
		goto done;
	}

	buf[n] = op->cmp_arg;

	*end_bits = op->u.stop.end_bits;

	*tail = op;

	op = op->next;
	n++;

	for (;;) {
		/* fetch */
		{
			if (op == NULL || op->instr != VM_OP_FETCH) {
				break;
			}

			assert(op->cmp == VM_CMP_ALWAYS);

			if (op->num_incoming > 0) {
				break;
			}

			if (op->u.fetch.end_bits != *end_bits) {
				break;
			}
		}

		op = op->next;

		/* stop */
		{
			if (op == NULL || op->instr != VM_OP_STOP) {
				break;
			}

			if (op->cmp != VM_CMP_NE) {
				break;
			}

			if (op->num_incoming > 0) {
				break;
			}

			if (op->u.stop.end_bits != *end_bits) {
				break;
			}
		}

		if (n >= len) {
			break;
		}

		buf[n] = op->cmp_arg;

		*tail = op;

		op = op->next;
		n++;
	}

done:

	return n;

unsuitable:

	return 0;
}

/* TODO: eventually to be non-static */
static int
fsm_print_cfrag(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops,
	const char *cp)
{
	struct dfavm_op_ir *op;

	assert(f != NULL);
	assert(opt != NULL);
	assert(retlist != NULL);
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

	assert(ops != NULL);

	/*
	 * Only declare variables if we're actually going to use them.
	 */
	if (ops->cmp == VM_CMP_ALWAYS && ops->instr == VM_OP_STOP) {
		assert(ops->next == NULL);
	} else {
		switch (opt->io) {
		case FSM_IO_GETC:
			fprintf(f, "\tint c;\n");
			break;

		case FSM_IO_STR:
			fprintf(f, "\tconst char *p;\n");
			fprintf(f, "\tint c;\n");
			fprintf(f, "\n");
			fprintf(f, "\tp = s;\n");
			break;

		case FSM_IO_PAIR:
			fprintf(f, "\tconst char *p;\n");
			fprintf(f, "\tint c;\n");
			fprintf(f, "\n");
			fprintf(f, "\tp = b;\n");
			break;
		}
	}

	for (op = ops; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			fprintf(f, "\n");
			print_label(f, op, opt);
			fprintf(f, "\n");
		}

		fprintf(f, "\t");

		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op, opt);
			if (-1 == print_end(f, op, opt, hooks, op->u.stop.end_bits)) {
				return -1;
			}
			break;

		case VM_OP_FETCH: {
			size_t n;
			enum dfavm_op_end end_bits;
			struct dfavm_op_ir *tail;

			char buf[8192];

			n = walk_sequence(op, &end_bits, buf, sizeof buf, &tail);

			if (n > 1 && opt->io == FSM_IO_PAIR) {
				fprintf(f, "if (e - p < %zu || 0 != memcmp(p, \"", n);
				escputbuf(f, opt, c_escputc_str, buf, n);
				fprintf(f, "\", %zu)) ", n);
				if (-1 == print_end(f, NULL, opt, hooks, end_bits)) {
					return -1;
				}
				fprintf(f, "\n");

				fprintf(f, "\t");
				fprintf(f, "p += %zu;\n", n);

				op = tail;
			} else if (n > 1 && opt->io == FSM_IO_STR) {
				fprintf(f, "if (0 != strncmp(p, \"");
				escputbuf(f, opt, c_escputc_str, buf, n);
				fprintf(f, "\", %zu)) ", n);
				if (-1 == print_end(f, NULL, opt, hooks, end_bits)) {
					return -1;
				}
				fprintf(f, "\n");

				fprintf(f, "\t");
				fprintf(f, "p += %zu;\n", n);

				op = tail;
			} else {
				print_fetch(f, opt);
				if (-1 == print_end(f, op, opt, hooks, op->u.fetch.end_bits)) {
					return -1;
				}
				if (opt->io == FSM_IO_PAIR) {
					/* second part of FSM_IO_PAIR fetch */
					fprintf(f, "\n\n\t");
					fprintf(f, "c = (unsigned char) *p++;");
				}
			}

			/*
			 * If the following instruction is an unconditional return,
			 * we won't be using this value.
			 */
			if (op->next != NULL && op->next->instr == VM_OP_STOP && op->next->cmp == VM_CMP_ALWAYS) {
				fprintf(f, "\n\t(void) c;\n");
			}
			break;
		}

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
fsm_print_vmc(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	const char *prefix;

	/* TODO: currently unused, but must be non-NULL */
	const char *cp = "";

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(retlist != NULL);

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (opt->fragment) {
		if (-1 == fsm_print_cfrag(f, opt, hooks, retlist, ops, cp)) {
			return -1;
		}
	} else {
		fprintf(f, "int\n%smain", prefix);
		fprintf(f, "(");

		switch (opt->io) {
		case FSM_IO_GETC:
			fprintf(f, "int (*fsm_getc)(void *opaque), void *getc_opaque");
			break;

		case FSM_IO_STR:
			fprintf(f, "const char *s");
			break;

		case FSM_IO_PAIR:
			fprintf(f, "const char *b, const char *e");
			break;

		default:
			assert(!"unreached");
			abort();
		}

		/*
		 * unsigned rather than fsm_end_id_t here, so the generated code
		 * doesn't depend on fsm.h
		 */
		switch (opt->ambig) {
		case AMBIG_NONE:
			break;

		case AMBIG_ERROR:
		case AMBIG_EARLIEST:
			fprintf(f, ",\n");
			fprintf(f, "\tconst unsigned *id");
			break;

		case AMBIG_MULTIPLE:
			fprintf(f, ",\n");
			fprintf(f, "\tconst unsigned **ids, size_t *count");
			break;

		default:
			assert(!"unreached");
			abort();
		}

		if (hooks->args != NULL) {
			fprintf(f, ",\n");
			fprintf(f, "\t");

			if (-1 == print_hook_args(f, opt, hooks, NULL, NULL)) {
				return -1;
			}
		}

		fprintf(f, ")\n");
		fprintf(f, "{\n");

		if (-1 == fsm_print_cfrag(f, opt, hooks, retlist, ops, cp)) {
			return -1;
		}

		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	return 0;
}
