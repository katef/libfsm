/*
 * Copyright 2019-2020 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include <re/re.h>

#include "ast.h"
#include "ast_analysis.h"
#include "ast_compile.h"

#if defined(__GNUC__) || defined(__clang__)
#define mul_overflow __builtin_umul_overflow
#else
static int
mul_overflow(unsigned x, unsigned y, unsigned *res)
{
	*res = x * y;
	return y != 0 && x != *res / y;
}
#endif

static int
rewrite(struct ast_expr_pool **poolp, struct ast_expr *n, enum re_flags flags);

static int
cmp(const void *_a, const void *_b)
{
	const struct ast_expr *a = * (const struct ast_expr * const *) _a;
	const struct ast_expr *b = * (const struct ast_expr * const *) _b;

	return ast_expr_cmp(a, b);
}

static int
cmp_uintptr_desc(const void *pa, const void *pb)
{
	const uintptr_t a = *(const uintptr_t *)pa;
	const uintptr_t b = *(const uintptr_t *)pb;
	return a < b ? 1 : a > b ? -1 : 0;
}

static int
contains_child(struct ast_expr **child, struct ast_expr **children, size_t count)
{
	return bsearch(child, children, count, sizeof(children[0]), cmp_uintptr_desc) != NULL;
}

static int
deduplicate_alt(struct ast_expr_pool **poolp, struct ast_expr *n)
{
	assert(n->type == AST_EXPR_ALT);
	assert(n->u.alt.count > 1);

	/* create a copy of the nodes, sorted by ast_expr_cmp */
	struct ast_expr **child_cp = calloc(n->u.alt.count, sizeof(n->u.alt.n[0]));
	if (child_cp == NULL) {
		return 0;
	}
	for (size_t i = 0; i < n->u.alt.count; i++) {
		child_cp[i] = n->u.alt.n[i];
	}
	qsort(child_cp, n->u.alt.count, sizeof *n->u.alt.n, cmp);

	/* free all but the first of each consecutive node that compares equal */
	size_t last_distinct = 0;
	for (size_t i = 1; i < n->u.alt.count; i++) {
		if (ast_expr_cmp(child_cp[last_distinct], child_cp[i]) == 0) {
			ast_expr_free(*poolp, child_cp[i]);
			child_cp[i] = NULL;
		} else {
			last_distinct = i;
		}
	}

	/* sort by pointer, since now we only care about pointer equality.
	 * sort descending, so all NULLs move to the end. */
	size_t surviving_count = n->u.alt.count;
	qsort(child_cp, surviving_count, sizeof(child_cp[0]), cmp_uintptr_desc);

	while (surviving_count > 0 && child_cp[surviving_count - 1] == NULL) {
		surviving_count--;
	}

	/* preserving order of the original child node array, shift them down
	 * to discard any node that no longer appears in child_cp[]. */
	size_t src = 0;
	size_t dst = 0;
	while (src < n->u.alt.count) {
		struct ast_expr *child = n->u.alt.n[src];
		assert(child != NULL);

		if (contains_child(&n->u.alt.n[src], child_cp, surviving_count)) {
			assert(dst <= src);
			if (dst != src) {
				n->u.alt.n[dst] = child;
			}
			dst++;	/* keep */
		}
		src++;
	}

	/* update the count to how many remain */
	n->u.alt.count = dst;
	free(child_cp);

	return 1;
}

static struct fsm *
compile_subexpr(struct ast_expr *e, enum re_flags flags)
{
	struct fsm *fsm;
	struct ast ast;

	/*
	 * We're compiling these expressions in isolation just for sake of
	 * set subtraction. In general we wouldn't know if these nodes inherit
	 * any flags from the parent (since we haven't yet run analysis for
	 * the parent node and its ancestors).
	 *
	 * However we do need to run ast_analysis() for sake of possible anchors
	 * (and whatever other complexities) within general sub-expressions,
	 * and so we provide flags as if these are top-level nodes.
	 *
	 * We're compiling implicitly anchored, just because it makes for
	 * simpler FSM.
	 */

	/* we're just setting these temporarily for sake of ast_analysis() */
	e->flags = AST_FLAG_FIRST | AST_FLAG_LAST;

	ast.expr = e;

	if (ast_analysis(&ast, flags) != AST_ANALYSIS_OK) {
		return 0;
	}

	fsm = ast_compile(&ast, flags | RE_ANCHORED | RE_NOCAPTURE, NULL, NULL);
	if (fsm == NULL) {
		return 0;
	}

	e->flags = 0x0;

	return fsm;
}

static int
rewrite_concat(struct ast_expr_pool **poolp, struct ast_expr *n, enum re_flags flags)
{
	static const struct ast_expr zero;
	size_t i;

	assert(n != NULL);
	assert(n->type == AST_EXPR_CONCAT);
	assert(n->flags == 0x0);

	for (i = 0; i < n->u.concat.count; i++) {
		if (!rewrite(poolp, n->u.concat.n[i], flags)) {
			return 0;
		}
	}

	/* Eliminate consecutive, redundant anchors */
	for (i = 0; i < n->u.concat.count; i++) {
		size_t j;
		if (n->u.concat.n[i]->type == AST_EXPR_ANCHOR) {
			const enum ast_anchor_type atype = n->u.concat.n[i]->u.anchor.type;
			j = i + 1;
			while (j < n->u.concat.count) {
				struct ast_expr *child = n->u.concat.n[j];
				if (child->type != AST_EXPR_ANCHOR
					|| child->u.anchor.type != atype) {
					break;
				}

				/* delete node */
				ast_expr_free(*poolp, n->u.concat.n[j]);
				if (j + 1 < n->u.concat.count) {
					memmove(&n->u.concat.n[j], &n->u.concat.n[j + 1],
					    (n->u.concat.count - j - 1) * sizeof *n->u.concat.n);
				}

				n->u.concat.count--;
				/* does not increment j, either loop iteration breaks
				 * or the current node is deleted and the rest shift down. */
			}
		}
	}

	/* a tombstone here means the entire concatenation is a tombstone */
	for (i = 0; i < n->u.concat.count; i++) {
		if (n->u.concat.n[i]->type == AST_EXPR_TOMBSTONE) {
			goto tombstone;
		}
	}

	/* remove empty children; these have no semantic effect */
	for (i = 0; i < n->u.concat.count; ) {
		if (n->u.concat.n[i]->type == AST_EXPR_EMPTY) {
			ast_expr_free(*poolp, n->u.concat.n[i]);

			if (i + 1 < n->u.concat.count) {
				memmove(&n->u.concat.n[i], &n->u.concat.n[i + 1],
					(n->u.concat.count - i - 1) * sizeof *n->u.concat.n);
			}

			n->u.concat.count--;
			continue;
		}

		i++;
	}

	/* Fold in concat children; these have to be in-situ, because order matters */
	for (i = 0; i < n->u.concat.count; ) {
		if (n->u.concat.n[i]->type == AST_EXPR_CONCAT) {
			struct ast_expr *dead = n->u.concat.n[i];
			size_t req_count;

			/* because of depth-first simplification */
			assert(dead->u.concat.count >= 1);

			req_count = n->u.concat.count + dead->u.concat.count - 1;

			if (n->u.concat.alloc < req_count) {
				void *tmp;

				tmp = realloc(n->u.concat.n, req_count * sizeof *n->u.concat.n);
				if (tmp == NULL) {
					return 0;
				}

				n->u.concat.n = tmp;

				n->u.concat.alloc = req_count;
			}

			/* move along our existing tail to make space */
			if (i + 1 < n->u.concat.count) {
				memmove(&n->u.concat.n[i + dead->u.concat.count], &n->u.concat.n[i + 1],
					(n->u.concat.count - i - 1) * sizeof *n->u.concat.n);
			}

			memcpy(&n->u.concat.n[i], dead->u.concat.n,
				dead->u.concat.count * sizeof *dead->u.concat.n);

			n->u.concat.count--;
			n->u.concat.count += dead->u.concat.count;

			dead->u.concat.count = 0;
			ast_expr_free(*poolp, dead);

			continue;
		}

		i++;
	}

	if (n->u.concat.count == 0) {
		free(n->u.concat.n);
		n->u.concat.n = NULL;

		goto empty;
	}

	if (n->u.concat.count == 1) {
		void *p = n->u.concat.n;
		struct ast_expr *q = n->u.concat.n[0];
		*n = *q;
		*q = zero;
		free(p);
		return 1;
	}

	return 1;

empty:

	n->type = AST_EXPR_EMPTY;

	return 1;

tombstone:

	for (i = 0; i < n->u.concat.count; i++) {
		ast_expr_free(*poolp, n->u.concat.n[i]);
	}

	free(n->u.concat.n);
	n->u.concat.n = NULL;

	n->type = AST_EXPR_TOMBSTONE;

	return 1;
}

static int
rewrite_alt(struct ast_expr_pool **poolp, struct ast_expr *n, enum re_flags flags)
{
	static const struct ast_expr zero;
	size_t i;

	assert(n != NULL);
	assert(n->type == AST_EXPR_ALT);
	assert(n->flags == 0x0);

	for (i = 0; i < n->u.alt.count; i++) {
		if (!rewrite(poolp, n->u.alt.n[i], flags)) {
			return 0;
		}
	}

	/*
	 * Empty children do have a semantic effect; for dialects with anchors,
	 * they affect the anchoring. So we cannot remove those in general here.
	 */
	/* TODO: multiple redundant empty nodes can be removed though, leaving just one */

	/* Fold in alt children; these do not have to be in-situ, because
	 * order doesn't matter here. But we do it anyway to be tidy */
	for (i = 0; i < n->u.alt.count; ) {
		if (n->u.alt.n[i]->type == AST_EXPR_ALT) {
			struct ast_expr *dead = n->u.alt.n[i];

			/* because of depth-first simplification */
			assert(dead->u.alt.count >= 1);

			if (n->u.alt.alloc < n->u.alt.count + dead->u.alt.count - 1) {
				void *tmp;

				tmp = realloc(n->u.alt.n,
					(n->u.alt.count + dead->u.alt.count - 1) * sizeof *n->u.alt.n);
				if (tmp == NULL) {
					return 0;
				}

				n->u.alt.n = tmp;

				n->u.alt.alloc = n->u.alt.count + dead->u.alt.count - 1;
			}

			/* move along our existing tail to make space */
			if (i + 1 < n->u.alt.count) {
				memmove(&n->u.alt.n[i + dead->u.alt.count], &n->u.alt.n[i + 1],
					(n->u.alt.count - i - 1) * sizeof *n->u.alt.n);
			}

			memcpy(&n->u.alt.n[i], dead->u.alt.n,
				dead->u.alt.count * sizeof *dead->u.alt.n);

			n->u.alt.count--;
			n->u.alt.count += dead->u.alt.count;

			dead->u.alt.count = 0;
			ast_expr_free(*poolp, dead);

			continue;
		}

		i++;
	}


	/* de-duplicate children */
	if (n->u.alt.count > 1) {
		if (!deduplicate_alt(poolp, n)) {
			return 0;
		}
	}

	if (n->u.alt.count == 0) {
		free(n->u.alt.n);
		n->u.alt.n = NULL;

		goto empty;
	}

	if (n->u.alt.count == 1) {
		void *p = n->u.alt.n;
		struct ast_expr *q = n->u.alt.n[0];
		*n = *q;
		*q = zero;
		free(p);
		return 1;
	}

	return 1;

empty:

	n->type = AST_EXPR_EMPTY;

	return 1;
}

static int
rewrite_repeat(struct ast_expr_pool **poolp, struct ast_expr *n, enum re_flags flags)
{
	assert(n != NULL);
	assert(n->type == AST_EXPR_REPEAT);
	assert(n->flags == 0x0);

	if (!rewrite(poolp, n->u.repeat.e, flags)) {
		return 0;
	}

	if (n->u.repeat.e->type == AST_EXPR_EMPTY) {
		ast_expr_free(*poolp, n->u.repeat.e);

		goto empty;
	}

	/*
	 * Should never be constructed, but just in case.
	 */
	if (n->u.repeat.min == 0 && n->u.repeat.max == 0) {
		ast_expr_free(*poolp, n->u.repeat.e);

		goto empty;
	}

	/*
	 * Should never be constructed, but just in case.
	 */
	if (n->u.repeat.min == 1 && n->u.repeat.max == 1) {
		struct ast_expr *dead = n->u.repeat.e;

		*n = *n->u.repeat.e;

		dead->type = AST_EXPR_EMPTY;
		ast_expr_free(*poolp, dead);

		return 1;
	}

	/*
	 * Fold together nested repetitions of the form a{h,i}{j,k} where
	 * the result is expressable as a single repetition node a{v,w}.
	 */
	if (n->u.repeat.e->type == AST_EXPR_REPEAT) {
		const struct ast_expr *inner, *outer;
		struct ast_expr *dead;
		unsigned h, i, j, k;
		unsigned v, w;

		inner = n->u.repeat.e;
		outer = n;

		h = inner->u.repeat.min;
		i = inner->u.repeat.max;
		j = outer->u.repeat.min;
		k = outer->u.repeat.max;

		assert(h != AST_COUNT_UNBOUNDED);
		assert(j != AST_COUNT_UNBOUNDED);

		/* Bail out (i.e. with no rewriting) on overflow */
		if (mul_overflow(h, j, &v)) {
			return 1;
		}
		if (i == AST_COUNT_UNBOUNDED || k == AST_COUNT_UNBOUNDED) {
			w = AST_COUNT_UNBOUNDED;
		} else if (mul_overflow(i, k, &w)) {
			return 1;
		}

		if (h == 0 || h == 1) {
			dead = n->u.repeat.e;

			n->u.repeat.min = v;
			n->u.repeat.max = w;
			n->u.repeat.e    = n->u.repeat.e->u.repeat.e;

			dead->type = AST_EXPR_EMPTY;
			ast_expr_free(*poolp, dead);

			return 1;
		}

		/*
		 * a{h,i}{j,k} is equivalent to a{h*j,i*k} if it's possible to combine,
		 * and it's possible iff the range of the result is not more than
		 * the sum of the ranges of the two inputs.
		 */
		if (i != AST_COUNT_UNBOUNDED && k != AST_COUNT_UNBOUNDED) {
			/* I don't know why this is true */
			assert(w - v > i - h + k - j);
		}

		/*
		 * a{h,}{j,}
		 * a{h,i}{j,}
		 * a{h,}{j,k}
		 */
		if ((i == AST_COUNT_UNBOUNDED || k == AST_COUNT_UNBOUNDED) && h <= 1 && j <= 1) {
			dead = n->u.repeat.e;

			n->u.repeat.min = v;
			n->u.repeat.max = w;
			n->u.repeat.e   = n->u.repeat.e->u.repeat.e;

			dead->type = AST_EXPR_EMPTY;
			ast_expr_free(*poolp, dead);

			return 1;
		}

		if (h > 1 && i == AST_COUNT_UNBOUNDED && j > 0) {
			dead = n->u.repeat.e;

			n->u.repeat.min = v;
			n->u.repeat.max = w;
			n->u.repeat.e   = n->u.repeat.e->u.repeat.e;

			dead->type = AST_EXPR_EMPTY;
			ast_expr_free(*poolp, dead);

			return 1;
		}

		return 1;
	}

	/*
	 * A nullable repetition of a tombstone can only match by the nullable
	 * option, which is equivalent to an empty node. A non-nullable
	 * repetition of a tombstone can never match, because that would require
	 * the tombstone to be traversed, so that is equivalent to a tombstone
	 * itself.
	 *
	 * This logic is repeated in a more generally-applicable way during the
	 * AST anchor analysis, to cater for unsatisfiable nodes we don't know
	 * during this rewriting pass yet. I'm keeping the more specific case
	 * here just because it helps with simplifying the tree first.
	 */

	if (n->u.repeat.min == 0 && n->u.repeat.e->type == AST_EXPR_TOMBSTONE) {
		ast_expr_free(*poolp, n->u.repeat.e);

		goto empty;
	}

	if (n->u.repeat.min > 0 && n->u.repeat.e->type == AST_EXPR_TOMBSTONE) {
		ast_expr_free(*poolp, n->u.repeat.e);

		goto tombstone;
	}

	return 1;

empty:

	n->type = AST_EXPR_EMPTY;

	return 1;

tombstone:

	n->type = AST_EXPR_TOMBSTONE;

	return 1;
}

static int
rewrite_subtract(struct ast_expr_pool **poolp, struct ast_expr *n, enum re_flags flags)
{
	int empty;

	assert(n != NULL);
	assert(n->type == AST_EXPR_SUBTRACT);
	assert(n->flags == 0x0);

	if (!rewrite(poolp, n->u.subtract.a, flags)) {
		return 0;
	}

	/* If the lhs operand is empty, the result is always empty */
	if (n->u.subtract.a->type == AST_EXPR_EMPTY) {
		ast_expr_free(*poolp, n->u.subtract.a);
		ast_expr_free(*poolp, n->u.subtract.b);

		goto empty;
	}

	if (!rewrite(poolp, n->u.subtract.b, flags)) {
		return 0;
	}

	/* TODO: If the rhs operand is 00-ff, the result is always empty
	 * (unless RE_UNICODE is set) */

	/* TODO: optimisation for computing subtractions for simple cases here;
	 * this should be possible by walking AST nodes directly (and sorting
	 * alt lists where neccessary) */

	/*
	 * This implementation of subtraction in terms of FSM is fallback for
	 * the general case; we will always want to support subtraction for
	 * arbitrary regular languages in general (as opposed to just the
	 * subsets our dialects currently construct for character classes),
	 * because eventually some dialect may offer an operator for
	 * subtracting arbitrary sub-expressions.
	 *
	 * This is all quite expensive.
	 *
	 * We always need to pass in user-supplied flags, because these
	 * sub-expressions may be written to assume particular flags
	 * (for example subtraction of two case-insensitive sets may depend
	 * on case-insensitity to produce an empty result).
	 */
	{
		struct fsm *a, *b;
		struct fsm *q;

		a = compile_subexpr(n->u.subtract.a, flags);
		if (a == NULL) {
			return 0;
		}

		b = compile_subexpr(n->u.subtract.b, flags);
		if (b == NULL) {
			fsm_free(a);
			return 0;
		}

		q = fsm_subtract(a, b);
		if (q == NULL) {
			return 0;
		}

		empty = fsm_empty(q);

		fsm_free(q);
	}

	if (empty) {
		ast_expr_free(*poolp, n->u.subtract.a);
		ast_expr_free(*poolp, n->u.subtract.b);
		goto tombstone;
	}

	return 1;

empty:

	n->type = AST_EXPR_EMPTY;

	return 1;

tombstone:

	n->type = AST_EXPR_TOMBSTONE;

	return 1;
}

static int
rewrite(struct ast_expr_pool **poolp, struct ast_expr *n, enum re_flags flags)
{
	if (n == NULL) {
		return 1;
	}

	/* flags haven't been set yet */
	assert(n->flags == 0x0);

	switch (n->type) {
	case AST_EXPR_EMPTY:
		return 1;

	case AST_EXPR_CONCAT:
		return rewrite_concat(poolp, n, flags);

	case AST_EXPR_ALT:
		return rewrite_alt(poolp, n, flags);

	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
		return 1;

	case AST_EXPR_REPEAT:
		return rewrite_repeat(poolp, n, flags);

	case AST_EXPR_GROUP:
		if (!rewrite(poolp, n->u.group.e, flags)) {
			return 0;
		}

		if (n->u.group.e->type == AST_EXPR_TOMBSTONE) {
			ast_expr_free(*poolp, n->u.group.e);

			goto tombstone;
		}

		return 1;

	case AST_EXPR_ANCHOR:
		return 1;

	case AST_EXPR_SUBTRACT:
		return rewrite_subtract(poolp, n, flags);

	case AST_EXPR_RANGE:
	case AST_EXPR_TOMBSTONE:
		return 1;

	default:
		assert(!"unreached");
	}

tombstone:

	n->type = AST_EXPR_TOMBSTONE;

	return 1;
}

int
ast_rewrite(struct ast *ast, enum re_flags flags)
{
	return rewrite(&ast->pool, ast->expr, flags);
}

