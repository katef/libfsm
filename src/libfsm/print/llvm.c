/*
 * Copyright 2008-2024 Katherine Flavel
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

#define OPAQUE_POINTERS 1

#ifdef OPAQUE_POINTERS // llvm >= 15
static const char *ptr_i8   = "ptr";
static const char *ptr_i32  = "ptr";
static const char *ptr_void = "ptr";
#else
static const char *ptr_i8   = "i8*";
static const char *ptr_i32  = "i32*";
static const char *ptr_void = "i8*";
#endif

/*
 * If we had a stack, the current set of live values would be a frame.
 * We're a DFA, so we don't have a stack. But I still think of them as a frame.
 */
struct frame {
	unsigned r;
	unsigned b;
	unsigned c;
};

static unsigned
use(const unsigned *n)
{
	assert(*n > 0);
	return *n - 1;
}

static unsigned
decl(unsigned *n)
{
	return (*n)++;
}

static int
leaf(FILE *f, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque)
{
	assert(f != NULL);
	assert(leaf_opaque == NULL);

	(void) ids;
	(void) count;
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
	case VM_CMP_LT: return "ult";
	case VM_CMP_LE: return "ule";
	case VM_CMP_EQ: return "eq";
	case VM_CMP_GE: return "uge";
	case VM_CMP_GT: return "ugt";
	case VM_CMP_NE: return "ne";

	case VM_CMP_ALWAYS:
	default:
		assert("unreached");
		return NULL;
	}
}

static void
print_decl(FILE *f, const char *name, unsigned n)
{
	fprintf(f, "\t%%%s%u = ", name, n);
}

static void
print_ret(FILE *f, long l)
{
	fprintf(f, "\tret i32 %ld\n", l);
}

// TODO: variadic
static void
print_label(FILE *f, const struct dfavm_op_ir *op, bool decl)
{
	if (!decl) {
		fprintf(f, "%%");
	}

	fprintf(f, "l%" PRIu32, op->index);

	if (decl) {
		fprintf(f, ":\n");
	}
}

static void
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
	struct frame *frame)
{
	assert(frame != NULL);
	assert(op->cmp != VM_CMP_ALWAYS);

	print_decl(f, "r", decl(&frame->r));
	fprintf(f, "icmp %s i8 %%c%u, ",
		cmp_operator(op->cmp), use(&frame->c));
	llvm_escputcharlit(f, opt, op->cmp_arg);
	fprintf(f, " ; ");
	c_escputcharlit(f, opt, op->cmp_arg); // C escaping for a comment
	fprintf(f, "\n");
}

static int
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
	const struct ir *ir, enum dfavm_op_end end_bits)
{
	if (end_bits == VM_END_FAIL) {
		print_ret(f, -1);
	} else {
		if (opt->endleaf != NULL) {
			if (-1 == opt->endleaf(f,
				op->ir_state->endids.ids, op->ir_state->endids.count,
				opt->endleaf_opaque))
			{
				return -1;
			}
		} else {
			print_ret(f, op->ir_state - ir->states);
		}
	}

	return 0;
}

static void
print_jump(FILE *f, const struct dfavm_op_ir *dest)
{
	fprintf(f, "\tbr label ");
	print_label(f, dest, false);
	fprintf(f, "\n");
}

static void
print_fetch(FILE *f, const struct fsm_options *opt,
	struct frame *frame, const struct dfavm_op_ir *next)
{
	unsigned n = decl(&frame->c);
	unsigned b = decl(&frame->b);

	assert(frame != NULL);
	assert(next != NULL);

	/*
	 * Emitting phi here would mean we'd need to keep track of the
	 * basic blocks that jump to the current block. That's entirely
	 * possible with our IR, but seems like a lot of work when we
	 * can just alloca and let llvm do it for us.
	 */

	// TODO: consider emitting fetch(%s, %n)

	switch (opt->io) {
	case FSM_IO_GETC: {
		print_decl(f, "i", n);
		fprintf(f, "tail call i32 %%fsm_getc(%s noundef %%getc_opaque) #1\n",
			ptr_i8);

		print_decl(f, "r", decl(&frame->r));
		fprintf(f, "icmp eq i32 %%i%u, -1 ; EOF\n", n);

		fprintf(f, "\tbr i1 %%r%u, label %%t%u, label %%f%u\n",
			use(&frame->r), b, b);
		fprintf(f, "f%u:\n", b);

		print_decl(f, "c", n);
		fprintf(f, "trunc i32 %%i%u to i8\n", n);

		print_jump(f, next);
		break;
	}

	case FSM_IO_STR: {
		/*
		 * If we increment %s instead of maintaining %n, we'd need to
		 * do it post-EOS check. Otherwise we'd skip the first character
		 * (or start with the pointer out of bounds).
		 */
		print_decl(f, "n", n);
		fprintf(f, "load i32, %s %%n\n", ptr_i32);

		print_decl(f, "n.new", n);
		fprintf(f, "add i32 1, %%n%u\n", n);

		fprintf(f, "\tstore i32 %%n.new%u, %s %%n\n", n, ptr_i32);

		print_decl(f, "p", n);
		fprintf(f, "getelementptr inbounds i8, %s %%s, i32 %%n%u\n",
			ptr_i8, n);

		print_decl(f, "c", n);
		fprintf(f, "load i8, %s %%p%u\n",
			ptr_i8, n);

		print_decl(f, "r", decl(&frame->r));
	  	fprintf(f, "icmp eq i8 %%c%u, 0 ; EOT\n",
			n);

		// TODO: skip t%u: if the next instruction is ret -1, centralise it to fail:
		fprintf(f, "\tbr i1 %%r%u, label %%t%u, label %%l%u\n",
			use(&frame->r), b, next->index);
		break;
	 }

	case FSM_IO_PAIR: {
		print_decl(f, "n", n);
		fprintf(f, "load i32, %s %%n\n", ptr_i32);

		print_decl(f, "n.new", n);
		fprintf(f, "add i32 1, %%n%u\n", n);

		fprintf(f, "\tstore i32 %%n.new%u, %s %%n\n", n, ptr_i32);

		print_decl(f, "p", n);
		fprintf(f, "getelementptr inbounds i8, %s %%b, i32 %%n%u\n",
			ptr_i8, n);

		print_decl(f, "r", decl(&frame->r));
	  	fprintf(f, "icmp eq %s %%p%u, %%e ; EOF\n",
			ptr_i8, n);

		fprintf(f, "\tbr i1 %%r%u, label %%t%u, label %%f%u\n",
			use(&frame->r), b, b);
		fprintf(f, "f%u:\n", b);

		print_decl(f, "c", n);
		fprintf(f, "load i8, %s %%p%u\n",
			ptr_i8, n);

		print_jump(f, next);
		break;
	}

	default:
		fprintf(stderr, "unsupported IO API\n");
		exit(EXIT_FAILURE);
	}

	// TODO: skip t%u: if the next instruction is ret -1, centralise it to fail:
	fprintf(f, "t%u:\n", use(&frame->b));
}

/* TODO: eventually to be non-static */
static int
fsm_print_llvmfrag(FILE *f, const struct dfavm_assembler_ir *a,
	const struct fsm_options *opt, const struct ir *ir,
	const char *cp,
	int (*leaf)(FILE *, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque),
	const void *leaf_opaque)
{
	struct dfavm_op_ir *op;

	assert(f != NULL);
	assert(a != NULL);
	assert(opt != NULL);
	assert(cp != NULL);

	/* TODO: we don't currently have .opaque information attached to struct dfavm_op_ir.
	 * We'll need that in order to be able to use the leaf callback here. */
	(void) leaf;
	(void) leaf_opaque;

	/* TODO: we'll need to heed cp for e.g. lx's codegen */
	(void) cp;

	{
		uint32_t l;

		l = 0;

		for (op = a->linked; op != NULL; op = op->next) {
			op->index = l++;
		}
	}

	print_jump(f, a->linked);

	struct frame frame = { 0, 0, 0 };
	for (op = a->linked; op != NULL; op = op->next) {
		print_label(f, op, true);

		if (op->ir_state != NULL && op->ir_state->example != NULL) {
			/* C's escaping seems to be a subset of llvm's, and these are
			 * for comments anyway. So I'm borrowing this for C here */
			fprintf(f, "\t; e.g. \"");
			escputs(f, opt, c_escputc_str, op->ir_state->example);
			fprintf(f, "\"");

			fprintf(f, "\n");
		}

		switch (op->instr) {
		case VM_OP_STOP:
			if (op->cmp != VM_CMP_ALWAYS) {
				unsigned b = decl(&frame.b);
				unsigned next = op->next->index;

				print_cond(f, op, opt, &frame);

// TODO: fold into print_cond()
				fprintf(f, "\tbr i1 %%r%u, label %%t%u, label %%l%u\n",
					use(&frame.r), b, next);
				fprintf(f, "t%u:\n", b);
			}

			if (-1 == print_end(f, op, opt, ir, op->u.stop.end_bits)) {
				return -1;
			}
			break;

		case VM_OP_FETCH: {
			print_fetch(f, opt, &frame, op->next);

			if (-1 == print_end(f, op, opt, ir, op->u.fetch.end_bits)) {
				return -1;
			}
			break;
		}

		case VM_OP_BRANCH: {
			const struct dfavm_op_ir *dest = op->u.br.dest_arg;

			if (op->cmp == VM_CMP_ALWAYS) {
				print_jump(f, dest);
			} else {
				unsigned next = op->next->index;

				print_cond(f, op, opt, &frame);

				fprintf(f, "\tbr i1 %%r%u, label ", use(&frame.r));
				print_label(f, dest, false);
				fprintf(f, ", label %%l%u\n", next);
			}
			break;
		}

		default:
			assert(!"unreached");
			break;
		}
	}

// TODO: collate ret values together at the end, keeps them out of the i-cache maybe
// or better yet, have one ret: label and emit a phi

	return 0;
}

static int
fsm_print_llvm_complete(FILE *f, const struct ir *ir,
	const struct fsm_options *opt, const char *prefix, const char *cp)
{
	static const struct dfavm_assembler_ir zero;
	struct dfavm_assembler_ir a;

	static const struct fsm_vm_compile_opts vm_opts = {
		FSM_VM_COMPILE_DEFAULT_FLAGS,
		FSM_VM_COMPILE_VM_V1,
		NULL
	};

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

	a = zero;

	if (!dfavm_compile_ir(&a, ir, vm_opts)) {
		return -1;
	}

	if (opt->fragment) {
		fsm_print_llvmfrag(f, &a, opt, ir, cp,
			opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque);
		goto error;
	}

	fprintf(f, "; generated\n");
	fprintf(f, "define dso_local i32 @%smain", prefix);

	switch (opt->io) {
	case FSM_IO_GETC:
#ifdef OPAQUE_POINTERS
		fprintf(f, "(i32 (%s)* nocapture noundef readonly %%fsm_getc, %s noundef %%getc_opaque)",
			ptr_i8, ptr_void);
#else
		fprintf(f, "(ptr nocapture noundef readonly %%fsm_getc, ptr noundef %%getc_opaque)");
#endif
		break;

	case FSM_IO_STR:
		fprintf(f, "(%s nocapture noundef readonly %%s)",
			ptr_i8);
		break;

	case FSM_IO_PAIR:
		fprintf(f, "(%s noundef readonly %%b, %s noundef readnone %%e)",
			ptr_i8, ptr_i8);
		break;

	default:
		fprintf(f, "(%s nocapture noundef readonly %%b, %s nocapture noundef readonly %%e)",
			ptr_i8, ptr_i8);
		break;
	}

	fprintf(f, " local_unnamed_addr");
	fprintf(f, " hot nosync nounwind norecurse willreturn");
	fprintf(f, "#0 {\n");

	switch (opt->io) {
	case FSM_IO_GETC:
		break;

	case FSM_IO_STR:
		fprintf(f, "\t%%n = alloca i32\n");
		fprintf(f, "\tstore i32 -1, %s %%n\n", ptr_i32);
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\t%%n = alloca i32\n");
		fprintf(f, "\tstore i32 -1, %s %%n\n", ptr_i32);
		break;

	default:
		fprintf(stderr, "unsupported IO API\n");
		exit(EXIT_FAILURE);
	}

	fsm_print_llvmfrag(f, &a, opt, ir, cp,
		opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque);

	fprintf(f, "}\n");
	fprintf(f, "\n");

	dfavm_opasm_finalize_op(&a);

	if (ferror(f)) {
		return -1;
	}

	return 0;

error:

	dfavm_opasm_finalize_op(&a);

	return -1;
}

int
fsm_print_llvm(FILE *f, const struct fsm *fsm)
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

	r = fsm_print_llvm_complete(f, ir, fsm->opt, prefix, cp);

	free_ir(fsm, ir);

	return r;
}

