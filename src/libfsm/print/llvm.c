/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
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

#define OPAQUE_POINTERS 1

#if OPAQUE_POINTERS // llvm >= 15
static const char *ptr_i8   = "ptr";
static const char *ptr_i32  = "ptr";
static const char *ptr_void = "ptr";
static const char *ptr_rt   = "ptr";
#else
static const char *ptr_i8   = "i8*";
static const char *ptr_i32  = "i32*";
static const char *ptr_void = "i8*";
static const char *ptr_rt   = "%rt*";
#endif

static const struct dfavm_op_ir fail; // used as a unqiue address only

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

static int
default_accept(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);

	(void) hook_opaque;
	(void) lang_opaque;

	switch (opt->ambig) {
	case AMBIG_NONE:
		fprintf(f, "%%rt true");
		break;

	case AMBIG_ERROR:
// TODO: decide if we deal with this ahead of the call to print or not
		if (state_metadata->end_id_count > 1) {
			errno = EINVAL;
			return -1;
		}

		fprintf(f, "%%rt { i1 true, i32 %u }", state_metadata->end_ids[0]);
		break;

	case AMBIG_EARLIEST:
		/*
		 * The libfsm api guarentees these ids are unique,
		 * and only appear once each, and are sorted.
		 */
		fprintf(f, "%%rt { i1 true, i32 %u }", state_metadata->end_ids[0]);
		break;

	case AMBIG_MULTIPLE:
		fprintf(f, "internal unnamed_addr constant [%zu x i32] [", state_metadata->end_id_count);
		for (size_t j = 0; j < state_metadata->end_id_count; j++) {
			fprintf(f, "i32 %u", state_metadata->end_ids[j]);
			if (j + 1 < state_metadata->end_id_count) {
				fprintf(f, ", ");
			}
		}
		fprintf(f, "]");
		break;

	default:
		assert(!"unreached");
		abort();
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

	switch (opt->ambig) {
	case AMBIG_NONE:
		fprintf(f, "%%rt false");
		break;

	case AMBIG_ERROR:
	case AMBIG_EARLIEST:
		fprintf(f, "%%rt { i1 false, i32 poison }");
		break;

	case AMBIG_MULTIPLE:
		fprintf(f, "%%rt { %s poison, i64 -1 }", ptr_i32);
		break;

	default:
		assert(!"unreached");
		abort();
	}

	if (opt->comments) {
		fprintf(f, " ; fail");
	}

	return 0;
}

static int
print_rettype(FILE *f, const char *name, enum fsm_ambig ambig)
{
	fprintf(f, "%s = type ", name);

	switch (ambig) {
	case AMBIG_NONE:
		fprintf(f, "i1");
		break;

	case AMBIG_ERROR:
	case AMBIG_EARLIEST:
		// success, id
		fprintf(f, "{ i1, i32 }");
		break;

	case AMBIG_MULTIPLE:
		// ids, -1/count
		fprintf(f, "{ %s, i64 }", ptr_i32);
		break;

	default:
		assert(!"unreached");
		abort();
	}

	fprintf(f, "\n");

	return 0;
}

static void
print_decl(FILE *f, const char *name, unsigned n)
{
	fprintf(f, "\t%%%s%u = ", name, n);
}

static void
vprint_label(FILE *f, bool decl, const char *fmt, va_list ap)
{
	assert(fmt != NULL);

	if (!decl) {
		fprintf(f, "label %%");
	}

	vfprintf(f, fmt, ap);

	if (decl) {
		fprintf(f, ":\n");
	}
}

static void
print_label(FILE *f, bool decl, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprint_label(f, decl, fmt, ap);
	va_end(ap);
}

static void
print_target(FILE *f, const struct dfavm_op_ir *op,
	const char *fmt, ...)
{
	va_list ap;

	if (op == &fail) {
		print_label(f, false, "fail");
	} else if (op != NULL && op->instr == VM_OP_STOP && op->cmp == VM_CMP_ALWAYS && op->u.stop.end_bits == VM_END_FAIL) {
		print_label(f, false, "fail");
	} else if (op != NULL) {
		print_label(f, false, "l%" PRIu32, op->index);
	} else if (fmt == NULL) {
		assert("!unreached");
		abort();
	} else {
		va_start(ap, fmt);
		vprint_label(f, false, fmt, ap);
		va_end(ap);
	}
}

static void
print_cond(FILE *f, const struct fsm_options *opt, struct dfavm_op_ir *op,
	struct frame *frame)
{
	assert(frame != NULL);
	assert(op->cmp != VM_CMP_ALWAYS);

	print_decl(f, "r", decl(&frame->r));
	fprintf(f, "icmp %s i8 %%c%u, ",
		cmp_operator(op->cmp), use(&frame->c));
	llvm_escputcharlit(f, opt, op->cmp_arg);

	if (opt->comments) {
		fprintf(f, " ; ");
		c_escputcharlit(f, opt, op->cmp_arg); // C escaping for a comment
	}
	fprintf(f, "\n");
}

static void
print_jump(FILE *f, const struct dfavm_op_ir *dest)
{
	fprintf(f, "\tbr ");
	print_target(f, dest, NULL);
	fprintf(f, "\n");
}

static void
print_incr(FILE *f, const char *name, unsigned n)
{
	// TODO: emit incr()?
	print_decl(f, "n.new", n);
	fprintf(f, "add i32 1, %%%s%u\n", name, n);

	fprintf(f, "\tstore i32 %%%s.new%u, %s %%%s\n", name, n, ptr_i32, name);
}

static void
print_branch(FILE *f, const struct frame *frame,
	const struct dfavm_op_ir *op_true, const struct dfavm_op_ir *op_false)
{
	fprintf(f, "\tbr i1 %%r%u, ", use(&frame->r));
	print_target(f, op_true, "t%u", use(&frame->b));
	fprintf(f, ", ");
	print_target(f, op_false, "f%u", use(&frame->b));
	fprintf(f, "\n");
}

static void
print_fetch(FILE *f, const struct fsm_options *opt,
	struct frame *frame, const struct dfavm_op_ir *next,
	enum dfavm_op_end end_bits)
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
		fprintf(f, "icmp eq i32 %%i%u, -1", n);
		if (opt->comments) {
			fprintf(f, " ; EOF");
		}
		fprintf(f, "\n");

// XXX: we don't distinguish error from eof
// https://github.com/katef/libfsm/issues/484
		print_branch(f, frame,
			end_bits == VM_END_FAIL ? &fail : NULL,
			NULL);
		print_label(f, true, "f%u", b);

		print_decl(f, "c", n);
		fprintf(f, "trunc i32 %%i%u to i8\n", n);

		print_jump(f, next);
		break;
	}

	case FSM_IO_STR: {
		// TODO: we could increment %s instead of maintaining %n here
		print_decl(f, "n", n);
		fprintf(f, "load i32, %s %%n\n", ptr_i32);

		print_decl(f, "p", n);
		fprintf(f, "getelementptr inbounds i8, %s %%s, i32 %%n%u\n",
			ptr_i8, n);

		print_decl(f, "c", n);
		fprintf(f, "load i8, %s %%p%u\n",
			ptr_i8, n);

		print_decl(f, "r", decl(&frame->r));
	  	fprintf(f, "icmp eq i8 %%c%u, 0", n);
		if (opt->comments) {
			fprintf(f, " ; EOT");
		}
		fprintf(f, "\n");

		print_branch(f, frame,
			end_bits == VM_END_FAIL ? &fail : NULL,
			NULL);
		print_label(f, true, "f%u", b);

		print_incr(f, "n", n);

		print_jump(f, next);
		break;
	 }

	case FSM_IO_PAIR: {
		print_decl(f, "n", n);
		fprintf(f, "load i32, %s %%n\n", ptr_i32);

		print_decl(f, "p", n);
		fprintf(f, "getelementptr inbounds i8, %s %%b, i32 %%n%u\n",
			ptr_i8, n);

		print_decl(f, "r", decl(&frame->r));
	  	fprintf(f, "icmp eq %s %%p%u, %%e", ptr_i8, n);
		if (opt->comments) {
			fprintf(f, " ; EOT");
		}
		fprintf(f, "\n");

		print_branch(f, frame,
			end_bits == VM_END_FAIL ? &fail : NULL,
			NULL);
		print_label(f, true, "f%u", b);

		print_decl(f, "c", n);
		fprintf(f, "load i8, %s %%p%u\n",
			ptr_i8, n);

		print_incr(f, "n", n);

		print_jump(f, next);
		break;
	}

	default:
		fprintf(stderr, "unsupported IO API\n");
		exit(EXIT_FAILURE);
	}

	if (end_bits != VM_END_FAIL) {
		// TODO: skip t%u: if the next instruction is ret -1, centralise it to fail:
		fprintf(f, "t%u:\n", use(&frame->b));
	}
}

/* TODO: eventually to be non-static */
static int
fsm_print_llvmfrag(FILE *f,
	const struct fsm_options *opt,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops,
	const char *cp,
	const char *prefix)
{
	struct dfavm_op_ir *op;

	assert(f != NULL);
	assert(opt != NULL);
	assert(retlist != NULL);
	assert(cp != NULL);
	assert(prefix != NULL);

	/* TODO: we'll need to heed cp for e.g. lx's codegen */
	(void) cp;

	{
		uint32_t l;

		l = 0;

		for (op = ops; op != NULL; op = op->next) {
			op->index = l++;
		}
	}

	{
		print_jump(f, ops);

		/*
		 * We're jumping to ret*: labels, and having each jump
		 * to a single stop: with a phi instruction.
		 *
		 * This looks like:
		 *
		 *  stop:
		 *      %i = phi i64
		 *        [0, %ret0],
		 *        [1, %ret1],
		 *        [2, %ret2],
		 *        [3, %fail]
		 *      %p = getelementptr inbounds [4 x %rt], [4 x %rt]* @fsm.r, i64 0, i64 %i
		 *      %ret = load %rt, ptr %p
		 *      ret %rt %ret
		 *  fail:
		 *      br label %stop
		 *  ret0:
		 *      br label %stop
		 *  ret1:
		 *      br label %stop
		 *  ret2:
		 *      br label %stop
		 *
		 * where @fsm.r is [4 x %rt] and %rt is the return type.
		 *
		 * And we jump to stop: via the ret*: labels rather than
		 * to a phi node directly. This helps for two reasons:
		 *
		 *  - We don't need to track every location in the DFA
		 *    that can stop. In other words, many basic blocks
		 *    jump to the same ret*: label.
		 *
		 *  - The number of basic blocks grows quadratically
		 *    (as DFA grow quadratically), but the set of endids
		 *    remains constant. So the phi list is small.
		 *
		 * llvm doesn't find this optimisation for us.
		 */
		print_label(f, true, "stop");

		fprintf(f, "\t%%i = phi i64\n");
		for (size_t i = 0; i < retlist->count; i++) {
			fprintf(f, "\t  [%zu, %%ret%zu],\n", i, i);
		}
		fprintf(f, "\t  [%zu, %%fail]\n", retlist->count);

		fprintf(f, "\t%%p = getelementptr inbounds [%zu x %%rt], [%zu x %%rt]* @%sr, i64 0, i64 %%i\n",
			retlist->count + 1, retlist->count + 1, prefix);
		fprintf(f, "\t%%ret = load %%rt, %s %%p\n", ptr_rt);
		fprintf(f, "\tret %%rt %%ret\n");

		print_label(f, true, "fail");
		fprintf(f, "\tbr ");
		print_label(f, false, "stop");
		fprintf(f, "\n");

		for (size_t i = 0; i < retlist->count; i++) {
			print_label(f, true, "ret%zu", i);
			fprintf(f, "\tbr ");
			print_label(f, false, "stop");
			fprintf(f, "\n");
		}
	}

	struct frame frame = { 0, 0, 0 };
	for (op = ops; op != NULL; op = op->next) {
		if (op->instr != VM_OP_STOP || op->cmp != VM_CMP_ALWAYS || op->u.stop.end_bits != VM_END_FAIL) {
			print_label(f, true, "l%" PRIu32, op->index);

			/*
			 * We only show examples when there's a label for the block,
			 * otherwise it's confusing with the conditionally elided
			 * optimisations per-instruction below, which can result in
			 * no block code being emitted for a particular vm op.
			 */
			if (op->example != NULL) {
				/* C's escaping seems to be a subset of llvm's, and these are
				 * for comments anyway. So I'm borrowing this for C here */
				fprintf(f, "\t; e.g. \"");
				escputs(f, opt, c_escputc_str, op->example);
				fprintf(f, "\"");

				fprintf(f, "\n");
			}
		}

		switch (op->instr) {
		case VM_OP_STOP:
			if (op->cmp != VM_CMP_ALWAYS) {
				unsigned b = decl(&frame.b);

// TODO: our ret%u: label goes in place of t%u here, which means we're replacing the NULL
				print_cond(f, opt, op, &frame);
				print_branch(f, &frame,
					op->u.stop.end_bits == VM_END_FAIL ? &fail : NULL,
					op->next);
				if (op->u.stop.end_bits != VM_END_FAIL) {
					print_label(f, true, "t%u", b);
				}
			}

			if (op->u.stop.end_bits == VM_END_FAIL) {
				/* handled above */
			} else {
				assert(retlist->count > 0);
				const struct ret *ret = op->ret;
				assert(ret != NULL);
				assert(ret >= retlist->a && ret <= (retlist->a + retlist->count));
				fprintf(f, "\tbr ");
				print_label(f, false, "ret%u", ret - retlist->a);
				fprintf(f, "\n");
			}
			break;

		case VM_OP_FETCH: {
			print_fetch(f, opt, &frame, op->next, op->u.fetch.end_bits);

			if (op->u.fetch.end_bits == VM_END_FAIL) {
				/* handled in print_fetch() */
			} else {
				assert(retlist->count > 0);
				const struct ret *ret = op->ret;
				assert(ret != NULL);
				assert(ret >= retlist->a && ret <= (retlist->a + retlist->count));
				fprintf(f, "\tbr ");
				print_label(f, false, "ret%u", ret - retlist->a);
				fprintf(f, "\n");
			}
			break;
		}

		case VM_OP_BRANCH: {
			const struct dfavm_op_ir *dest = op->u.br.dest_arg;

			if (op->cmp == VM_CMP_ALWAYS) {
				print_jump(f, dest);
			} else {
				print_cond(f, opt, op, &frame);
				print_branch(f, &frame,
					dest,
					op->next);
			}
			break;
		}

		default:
			assert(!"unreached");
			break;
		}
	}

	return 0;
}

int
fsm_print_llvm(FILE *f,
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
		prefix = "fsm.";
	}

	if (hooks->cp != NULL) {
		cp = hooks->cp;
	} else {
		cp = "c"; /* XXX */
	}

	if (opt->fragment) {
		fsm_print_llvmfrag(f, opt, retlist, ops, cp, prefix);
		return 0;
	}

	fprintf(f, "; generated\n");
	print_rettype(f, "%rt", opt->ambig);

	/*
	 * For AMBIG_MULTIPLE we emit a bunch of arrays and then point at them from
	 * each %rt. So we call the hook for the arrays, because that's where the id
	 * list is. For other ambig modes, we call the hook for the %rt instead.
	 */
	if (opt->ambig == AMBIG_MULTIPLE) {
		for (size_t i = 0; i < retlist->count; i++) {
			fprintf(f, "@%sr%zu = ", prefix, i);
			const struct fsm_state_metadata state_metadata = {
				.end_ids = retlist->a[i].ids,
				.end_id_count = retlist->a[i].count,
			};
			if (-1 == print_hook_accept(f, opt, hooks,
				&state_metadata,
				default_accept, NULL))
			{
				return -1;
			}

			if (-1 == print_hook_comment(f, opt, hooks,
				&state_metadata))
			{
				return -1;
			}

			fprintf(f, "\n");
		}
	}

	fprintf(f, "@%sr = internal unnamed_addr constant [%zu x %%rt] [\n", prefix, retlist->count + 1);
	for (size_t i = 0; i < retlist->count; i++) {
		fprintf(f, "\t  ");
		if (opt->ambig == AMBIG_MULTIPLE) {
			fprintf(f, "%%rt { %s bitcast ([%zu x i32]* @%sr%zu to %s), i64 %zu }",
				ptr_i32, retlist->a[i].count, prefix, i, ptr_i32, retlist->a[i].count);
			fprintf(f, ",");
		} else {
			const struct fsm_state_metadata state_metadata = {
				.end_ids = retlist->a[i].ids,
				.end_id_count = retlist->a[i].count,
			};
			if (-1 == print_hook_accept(f, opt, hooks,
				&state_metadata,
				default_accept, NULL))
			{
				return -1;
			}

			fprintf(f, ",");

			if (-1 == print_hook_comment(f, opt, hooks,
				&state_metadata))
			{
				return -1;
			}
		}
		fprintf(f, "\n");
	}
	fprintf(f, "\t  ");
	if (-1 == print_hook_reject(f, opt, hooks, default_reject, NULL)) {
		return -1;
	}
	fprintf(f, "\n");
	fprintf(f, "\t]\n");

	fprintf(f, "define dso_local %%rt @%smain", prefix);

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
	fprintf(f, " #0 {\n");

	switch (opt->io) {
	case FSM_IO_GETC:
		break;

	case FSM_IO_STR:
		fprintf(f, "\t%%n = alloca i32\n");
		fprintf(f, "\tstore i32 0, %s %%n\n", ptr_i32);
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\t%%n = alloca i32\n");
		fprintf(f, "\tstore i32 0, %s %%n\n", ptr_i32);
		break;

	default:
		fprintf(stderr, "unsupported IO API\n");
		exit(EXIT_FAILURE);
	}

	fsm_print_llvmfrag(f, opt, retlist, ops, cp, prefix);

	fprintf(f, "}\n");
	fprintf(f, "\n");

	return 0;
}

