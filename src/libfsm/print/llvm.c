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

static int
print_leaf(FILE *f, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque)
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
	fprintf(f, " ; ");
	c_escputcharlit(f, opt, op->cmp_arg); // C escaping for a comment
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
		fprintf(f, "icmp eq i32 %%i%u, -1 ; EOF\n", n);

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
	  	fprintf(f, "icmp eq i8 %%c%u, 0 ; EOT\n",
			n);

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
	  	fprintf(f, "icmp eq %s %%p%u, %%e ; EOF\n",
			ptr_i8, n);

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

struct ret {
	size_t count;
	const fsm_end_id_t *ids;
	const struct ir_state *ir_state; // TODO: remove when we return endids only
};

struct ret_list {
	size_t count;
	struct ret *a;
};

static bool
append_ret(struct ret_list *list,
	const fsm_end_id_t *ids, size_t count,
	const struct ir_state *ir_state)
{
	const size_t low    = 16; /* must be power of 2 */
	const size_t factor =  2; /* must be even */

	assert(list != NULL);

	if (list->count == 0) {
		list->a = malloc(low * sizeof *list->a);
		if (list->a == NULL) {
			return false;
		}
	} else if (list->count >= low && (list->count & (list->count - 1)) == 0) {
		void *tmp;
		size_t new = list->count * factor;
		if (new < list->count) {
			errno = E2BIG;
			perror("realloc");
			exit(EXIT_FAILURE);
		}

		tmp = realloc(list->a, new * sizeof *list->a);
		if (tmp == NULL) {
			return false;
		}

		list->a = tmp;
	}

	list->a[list->count].ids = ids;
	list->a[list->count].count = count;

	/* .ir_state is not part of the "key" for struct ret,
	 * we ignore it for comparisons and effectively pick
	 * an arbitrary ir_state for output */
	list->a[list->count].ir_state = ir_state;

	list->count++;

	return true;
}

static int
cmp_ret_by_endid(const void *pa, const void *pb)
{
	const struct ret *a = pa;
	const struct ret *b = pb;

	if (a->count < b->count) { return -1; }
	if (a->count > b->count) { return +1; }

	/* .ir_state intentionally ignored */

	return memcmp(a->ids, b->ids, a->count * sizeof *a->ids);
}

static int
cmp_ret_by_state(const void *pa, const void *pb)
{
	const struct ret *a = pa;
	const struct ret *b = pb;

	if (a->ir_state < b->ir_state) { return -1; }
	if (a->ir_state > b->ir_state) { return +1; }

	/* .ids intentionally ignored */

	return 0;
}

static struct ret *
find_ret(const struct ret_list *list, const struct dfavm_op_ir *op,
	int (*cmp)(const void *pa, const void *pb))
{
	struct ret key;

	assert(op != NULL);
	assert(cmp != NULL);

	key.count    = op->ir_state->endids.count;
	key.ids      = op->ir_state->endids.ids;
	key.ir_state = op->ir_state;

	return bsearch(&key, list->a, list->count, sizeof *list->a, cmp);
}

static bool
build_retlist(struct ret_list *list, const struct dfavm_op_ir *a)
{
	const struct dfavm_op_ir *op;

	assert(list != NULL);

	for (op = a; op != NULL; op = op->next) {
		switch (op->instr) {
		case VM_OP_STOP:
			if (op->u.stop.end_bits == VM_END_FAIL) {
				/* %fail is special, don't add to retlist */
				continue;
			}

			break;

		case VM_OP_FETCH:
			if (op->u.fetch.end_bits == VM_END_FAIL) {
				/* %fail is special, don't add to retlist */
				continue;
			}

			break;

		case VM_OP_BRANCH:
			continue;

		default:
			assert(!"unreached");
			abort();
		}

		if (!append_ret(list,
			op->ir_state->endids.ids, op->ir_state->endids.count,
			op->ir_state))
		{
			return false;
		}
	}

	return true;
}

/* TODO: eventually to be non-static */
static int
fsm_print_llvmfrag(FILE *f, const struct fsm_options *opt,
	const struct ir *ir, struct dfavm_op_ir *ops,
	const char *cp,
	int (*leaf)(FILE *, const fsm_end_id_t *ids, size_t count, const void *leaf_opaque),
	const void *leaf_opaque)
{
	struct ret_list retlist;
	struct dfavm_op_ir *op;

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

	{
		uint32_t l;

		l = 0;

		for (op = ops; op != NULL; op = op->next) {
			op->index = l++;
		}
	}

	{
		retlist.count = 0;
		build_retlist(&retlist, ops);

		/* sort for both dedup and bsearch */
		qsort(retlist.a, retlist.count, sizeof *retlist.a,
			opt->endleaf != NULL ? cmp_ret_by_endid : cmp_ret_by_state);

		/*
		 * If we're going by endleaf, we deal with endids.
		 * If we don't have endleaf, we deal with state numbers.
		 */
		if (opt->endleaf != NULL && retlist.count > 0) {
			size_t j = 0;

			/* deduplicate based on endids only.
			 * j is the start of a run; i increments until we find
			 * the start of the next run */
			for (size_t i = 1; i < retlist.count; i++) {
				assert(i > j);
				if (cmp_ret_by_endid(&retlist.a[j], &retlist.a[i]) == 0) {
					continue;
				}

				j++;
				retlist.a[j] = retlist.a[i];
			}

			retlist.count = j + 1;

			assert(retlist.count > 0);
		}

		print_jump(f, ops);

		/*
		 * We're jumping to ret*: labels, and having each jump
		 * to a single stop: with a phi instruction.
		 *
		 * This looks like:
		 *
		 *  stop:
		 *      %ret = phi i32
		 *       [u0x1, %ret0], ; "abc"
		 *       [u0x2, %ret1], ; "xyz"
		 *       [u0x3, %ret2], ; "abc", "xyz"
		 *       [-1, %fail]
		 *      ret i32 %ret
		 *  fail:
		 *      br label %stop
		 *  ret0:
		 *      br label %stop
		 *  ret1:
		 *      br label %stop
		 *  ret2:
		 *      br label %stop
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
		fprintf(f, "\t%%ret = phi i32\n");
		for (size_t i = 0; i < retlist.count; i++) {
			fprintf(f, "\t  ");

			if (opt->endleaf != NULL) {
				if (-1 == opt->endleaf(f,
					retlist.a[i].ids, retlist.a[i].count,
					opt->endleaf_opaque))
				{
					return -1;
				}
			} else {
				fprintf(f, "[%td, %%ret%zu],\n",
					retlist.a[i].ir_state - ir->states, i);
			}
		}
		fprintf(f, "\t%s[-1, %%fail]\n", "  ");
		fprintf(f, "\tret i32 %%ret\n");

		print_label(f, true, "fail");
		fprintf(f, "\tbr ");
		print_label(f, false, "stop");
		fprintf(f, "\n");

		for (size_t i = 0; i < retlist.count; i++) {
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
		}

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
				assert(retlist.count > 0);
				const struct ret *ret = find_ret(&retlist, op,
					opt->endleaf != NULL ? cmp_ret_by_endid : cmp_ret_by_state);
				assert(ret != NULL);
				assert(ret >= retlist.a && ret <= (retlist.a + retlist.count));
				assert(ret->count == op->ir_state->endids.count);
				assert(0 == memcmp(ret->ids, op->ir_state->endids.ids, ret->count));
				fprintf(f, "\tbr ");
				print_label(f, false, "ret%u", ret - retlist.a);
				fprintf(f, "\n");
			}
			break;

		case VM_OP_FETCH: {
			print_fetch(f, opt, &frame, op->next, op->u.fetch.end_bits);

			if (op->u.fetch.end_bits == VM_END_FAIL) {
				/* handled in print_fetch() */
			} else {
				assert(retlist.count > 0);
				const struct ret *ret = find_ret(&retlist, op,
					opt->endleaf != NULL ? cmp_ret_by_endid : cmp_ret_by_state);
				assert(ret != NULL);
				assert(ret >= retlist.a && ret <= (retlist.a + retlist.count));
				assert(ret->count == op->ir_state->endids.count);
				assert(0 == memcmp(ret->ids, op->ir_state->endids.ids, ret->count));
				fprintf(f, "\tbr ");
				print_label(f, false, "ret%u", ret - retlist.a);
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

	if (retlist.count > 0) {
		free(retlist.a);
	}

	return 0;
}

int
fsm_print_llvm(FILE *f, const struct fsm_options *opt,
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
		fsm_print_llvmfrag(f, opt, ir, ops, cp,
			leaf, opt->leaf_opaque);
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

	fsm_print_llvmfrag(f, opt, ir, ops, cp,
		leaf, opt->leaf_opaque);

	fprintf(f, "}\n");
	fprintf(f, "\n");

	return 0;

error:

	return -1;
}

