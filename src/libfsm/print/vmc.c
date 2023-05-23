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
	fprintf(f, "l%" PRIu32 ":", op->index);

	if (op->ir_state->example != NULL) {
		fprintf(f, " /* e.g. \"");
		escputs(f, opt, c_escputc_str, op->ir_state->example);
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
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
	enum dfavm_op_end end_bits, const struct ir *ir)
{
	if (end_bits == VM_END_FAIL) {
		fprintf(f, "return -1;");
		return 0;
	}

	if (opt->endleaf != NULL) {
		if (-1 == opt->endleaf(f, op->ir_state->end_ids, opt->endleaf_opaque)) {
			return -1;
		}
	} else {
		fprintf(f, "return %td;", op->ir_state - ir->states);
	}

	return 0;
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
		fprintf(f, "if (c = fsm_getc(opaque), c == EOF) ");
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
fsm_print_cfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt,
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

	assert(a.linked != NULL);

	/*
	 * Only declare variables if we're actually going to use them.
	 */
	if (a.linked->cmp == VM_CMP_ALWAYS && a.linked->instr == VM_OP_STOP) {
		assert(a.linked->next == NULL);
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

	for (op = a.linked; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			fprintf(f, "\n");
			print_label(f, op, opt);
			fprintf(f, "\n");
		}

		fprintf(f, "\t");

		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op, opt);
			if (-1 == print_end(f, op, opt, op->u.stop.end_bits, ir)) {
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
				if (-1 == print_end(f, NULL, opt, end_bits, ir)) {
					return -1;
				}
				fprintf(f, "\n");

				fprintf(f, "\t");
				fprintf(f, "p += %zu;\n", n);

				op = tail;
			} else {
				print_fetch(f, opt);
				if (-1 == print_end(f, op, opt, op->u.fetch.end_bits, ir)) {
					return -1;
				}
				if (opt->io == FSM_IO_PAIR) {
					/* second part of FSM_IO_PAIR fetch */
					fprintf(f, "\n\n\t");
					fprintf(f, "c = (unsigned char) *p++;");
				}
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

	dfavm_opasm_finalize_op(&a);

	return 0;
}

static int
fsm_print_c_complete(FILE *f, const struct ir *ir, const struct fsm_options *opt, const char *prefix)
{
	/* TODO: currently unused, but must be non-NULL */
	const char *cp = "";

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);
	assert(prefix != NULL);

	if (opt->fragment) {
		if (-1 == fsm_print_cfrag(f, ir, opt, cp,
			opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque))
		{
			return -1;
		}
	} else {
		fprintf(f, "#include <string.h>\n");
		fprintf(f, "\n");

		fprintf(f, "int\n%smain", prefix);

		switch (opt->io) {
		case FSM_IO_GETC:
			fprintf(f, "(int (*fsm_getc)(void *opaque), void *opaque)\n");
			fprintf(f, "{\n");
			break;

		case FSM_IO_STR:
			fprintf(f, "(const char *s)\n");
			fprintf(f, "{\n");
			break;

		case FSM_IO_PAIR:
			fprintf(f, "(const char *b, const char *e)\n");
			fprintf(f, "{\n");
			break;
		}

		if (-1 == fsm_print_cfrag(f, ir, opt, cp,
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
fsm_print_vmc(FILE *f, const struct fsm *fsm)
{
	struct ir *ir;
	const char *prefix;
	int r;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return -1;
	}

	/* henceforth, no function should be passed struct fsm *, only the ir and options */

	if (fsm->opt->prefix != NULL) {
		prefix = fsm->opt->prefix;
	} else {
		prefix = "fsm_";
	}

	r = fsm_print_c_complete(f, ir, fsm->opt, prefix);

	free_ir(fsm, ir);

	return r;
}

