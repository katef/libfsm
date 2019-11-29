/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <re/re.h>

#include "class.h"
#include "print.h"
#include "ast.h"
#include "ast_analysis.h"

#define LOG_CONCAT_FLAGS 0

struct analysis_env {
	unsigned group_id;
};

struct anchoring_env {
	char past_any_consuming;
	char followed_by_consuming;
};

static int
is_nullable(const struct ast_expr *n)
{
	if (n->type == AST_EXPR_EMPTY) {
		return 1;
	}

	return n->flags & AST_FLAG_NULLABLE;
}

static int
is_start_anchor(const struct ast_expr *n)
{
	return n->type == AST_EXPR_ANCHOR && n->u.anchor.type == AST_ANCHOR_START;
}

static int
is_end_anchor(const struct ast_expr *n)
{
	return n->type == AST_EXPR_ANCHOR && n->u.anchor.type == AST_ANCHOR_END;
}

static void
set_flags(struct ast_expr *n, enum ast_flags flags)
{
	n->flags |= flags;
}

static enum ast_analysis_res
analysis_iter(struct analysis_env *env, struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_EMPTY:
		set_flags(n, AST_FLAG_NULLABLE);
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_RANGE:
	case AST_EXPR_NAMED:
		/* no special handling */
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			if (is_nullable(n)) {
				set_flags(n->u.concat.n[i], AST_FLAG_NULLABLE);
			}
			analysis_iter(env, n->u.concat.n[i]);
		}

		break;
	}

	case AST_EXPR_ALT: {
		size_t i;

		for (i = 0; i < n->u.alt.count; i++) {
			analysis_iter(env, n->u.alt.n[i]);
			/* spread nullability upward */
			if (is_nullable(n->u.alt.n[i])) {
				set_flags(n, AST_FLAG_NULLABLE);
			}
		}

		if (is_nullable(n)) { /* spread nullability downward */
			for (i = 0; i < n->u.alt.count; i++) {
				set_flags(n->u.alt.n[i], AST_FLAG_NULLABLE);
			}
		}
		break;
	}

	case AST_EXPR_REPEATED: {
		struct ast_expr *e = n->u.repeated.e;
		assert(e != NULL);

		if (n->u.repeated.low == 0) {
			set_flags(n, AST_FLAG_NULLABLE);
		}

		if (is_nullable(n)) {
			set_flags(e, AST_FLAG_NULLABLE);
		}

		analysis_iter(env, e);

		if (is_nullable(e)) {
			set_flags(n, AST_FLAG_NULLABLE);
		}
		break;
	}

	case AST_EXPR_GROUP: {
		struct ast_expr *e = n->u.group.e;

		if (is_nullable(n)) {
			set_flags(e, AST_FLAG_NULLABLE);
		}

		/* assign group ID */
		env->group_id++;
		n->u.group.id = env->group_id;

		analysis_iter(env, e);

		if (is_nullable(e)) {
			set_flags(n, AST_FLAG_NULLABLE);
		}
		break;
	}

	case AST_EXPR_FLAGS:
		set_flags(n, AST_FLAG_NULLABLE); 
		break;

	case AST_EXPR_ANCHOR:
		/* anchor flags will be handled on the second pass */
		break;

	case AST_EXPR_SUBTRACT:
		/* XXX: not sure */
		break;

	default:
		assert(!"unreached");
	}

	return AST_ANALYSIS_OK;
}

static int
always_consumes_input(const struct ast_expr *n, int thud)
{
	if (is_nullable(n)) {
		return 0;
	}

	switch (n->type) {
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_RANGE:
	case AST_EXPR_NAMED:
		return 1;

	case AST_EXPR_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			if (always_consumes_input(n->u.concat.n[i], thud)) {
				return 1;
			}
		}

		return 0;
	}

	case AST_EXPR_ALT: {
		size_t i;

		for (i = 0; i < n->u.alt.count; i++) {
			if (!always_consumes_input(n->u.alt.n[i], thud)) {
				return 0;
			}
		}

		return 1;
	}

	case AST_EXPR_REPEATED:
		/* not nullable, so check the repeated node */
		return always_consumes_input(n->u.repeated.e, thud);

	case AST_EXPR_ANCHOR:
		return 0;

	case AST_EXPR_GROUP:
		if (thud) {
			return 1; /* FIXME */
		}

		return always_consumes_input(n->u.group.e, thud);

	default:
		return 0;
	}
}

static enum ast_analysis_res
analysis_iter_anchoring(struct anchoring_env *env, struct ast_expr *n)
{
	enum ast_analysis_res res;

	/*
	 * Flag the overall RE as unsatisfiable if there are anchors
	 * that cannot be simultaneously satisfied.
	 */

	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_FLAGS:
		break;

	case AST_EXPR_ANCHOR:
		switch (n->u.anchor.type) {
		case AST_ANCHOR_START:
			/*
			 * If it's not possible to get here without consuming
			 * any input and there's a start anchor, the regex is
			 * inherently unsatisfiable.
			 */

			if (env->past_any_consuming) {
				/*
				 * Start anchors that are not in a FIRST position
				 * almost perfectly match unsatisfiable start
				 * anchors, with the exception of multiple start
				 * anchors in a row (e.g. `^^ab$`). We can't just
				 * treat them as nullable either, because it messes
				 * up the results for linking.
				 */

				set_flags(n, AST_FLAG_UNSATISFIABLE);
				assert(0 == (n->flags & AST_FLAG_FIRST));
				return AST_ANALYSIS_UNSATISFIABLE;
			}

			break;

		case AST_ANCHOR_END:
			if (env->followed_by_consuming) {
				/*
				 * See above: This is a near perfect subset of
				 * unsatisfiable cases, but fails to handle
				 * redundant $ anchors correctly (including
				 * ones with arbitrary nesting).
				 */

				assert(0 == (n->flags & AST_FLAG_LAST));
				set_flags(n, AST_FLAG_UNSATISFIABLE);
				return AST_ANALYSIS_UNSATISFIABLE;
			}

			break;

		default:
			assert(!"unreached");
		}
		break;

	/*
	 * These are the types that actually consume input.
	 */

	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_RANGE:
	case AST_EXPR_NAMED:
		if (!is_nullable(n)) {
			env->past_any_consuming = 1;
		}
		break;

	case AST_EXPR_CONCAT: {
		size_t i, j;
		char bak_consuming_after;

		bak_consuming_after = 0;

		for (i = 0; i < n->u.concat.count; i++) {
			struct ast_expr *child;

			child = n->u.concat.n[i];

#if LOG_CONCAT_FLAGS
			fprintf(stderr, "%s: %p: %lu: %p -- past_any %d\n",
			    __func__, (void *)n, i, (void *)child,
			    env->past_any_consuming);
#endif

			/*
			 * Before recurring, save and restore whether this is
			 * followed by anything which always consumes input.
			 *
			 * TODO: instead of doing a backward sweep for each node,
			 * do a reverse falsification step as a separate pass?
			 */
			bak_consuming_after = env->followed_by_consuming;

			/*
			 * If we don't already know whether this is being followed
			 * by something that always consumes input, check.
			 */
			if (!env->followed_by_consuming) {
				for (j = i + 1; j < n->u.concat.count; j++) {
					if (always_consumes_input(n->u.concat.n[j], 0)) {
						env->followed_by_consuming = 1;
						break;
					}
				}
			}

			res = analysis_iter_anchoring(env, child);

			env->followed_by_consuming = bak_consuming_after;

			if (res != AST_ANALYSIS_OK) {
				return res;
			}
		}

		break;
	}

	case AST_EXPR_ALT: {
		int any_sat;
		size_t i;

		any_sat = 0;

		for (i = 0; i < n->u.alt.count; i++) {
			struct anchoring_env bak;

			memcpy(&bak, env, sizeof(*env));

			res = analysis_iter_anchoring(env, n->u.alt.n[i]);
			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				/* prune unsatisfiable branch */
				struct ast_expr *doomed = n->u.alt.n[i];
				n->u.alt.n[i] = ast_expr_tombstone;
				ast_expr_free(doomed);
				continue;
			} else if (res == AST_ANALYSIS_OK) {
				any_sat = 1;
			} else {
				return res;
			}

			/* restore */
			memcpy(env, &bak, sizeof(*env));
		}

		/* An ALT group is only unstaisfiable if they ALL are. */
		if (!any_sat) {
			set_flags(n, AST_FLAG_UNSATISFIABLE);
			return AST_ANALYSIS_UNSATISFIABLE;
		}
		break;
	}

	case AST_EXPR_REPEATED:
		res = analysis_iter_anchoring(env, n->u.repeated.e);
		if (res != AST_ANALYSIS_OK) {
			return res;
		}
		break;

	case AST_EXPR_GROUP:
		res = analysis_iter_anchoring(env, n->u.group.e);
		if (res != AST_ANALYSIS_OK) {
			return res;
		}
		break;

	case AST_EXPR_SUBTRACT:
		res = analysis_iter_anchoring(env, n->u.subtract.a);
		if (res != AST_ANALYSIS_OK) {
			return res;
		}

		res = analysis_iter_anchoring(env, n->u.subtract.b);
		if (res != AST_ANALYSIS_OK) {
			return res;
		}
		break;

	default:
		assert(!"unreached");
	}

	return AST_ANALYSIS_OK;
}

static void
assign_firsts(struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_FLAGS:
		break;

	case AST_EXPR_ANCHOR:
		if (n->u.anchor.type == AST_ANCHOR_START) {
			set_flags(n, AST_FLAG_FIRST);
		}
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_RANGE:
	case AST_EXPR_NAMED:
		set_flags(n, AST_FLAG_FIRST);
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		set_flags(n, AST_FLAG_FIRST);
		for (i = 0; i < n->u.concat.count; i++) {
			struct ast_expr *child = n->u.concat.n[i];
			assign_firsts(child);

			if (always_consumes_input(child, 1) || is_start_anchor(child)) {
				break;
			}
		}
		break;
	}

	case AST_EXPR_ALT: {
		size_t i;

		set_flags(n, AST_FLAG_FIRST);
		for (i = 0; i < n->u.alt.count; i++) {
			assign_firsts(n->u.alt.n[i]);
		}
		break;
	}

	case AST_EXPR_REPEATED:
		set_flags(n, AST_FLAG_FIRST);
		/* Don't recurse.
		 *
		 * XXX - Not sure that this is the correct way to handle this.
		 *
		 * Recursing causes errors in the NFA when the REPEATED node is
		 * linked to the global self-loop (ie: it's either the first or
		 * last node for an unanchored RE.  Specifically, when the
		 * subexpression is compiled, the links to the global self-loop
		 * are created, which the REPEATED node then copies.
		 */
		break;

	case AST_EXPR_GROUP:
		set_flags(n, AST_FLAG_FIRST);
		assign_firsts(n->u.group.e);
		break;

	case AST_EXPR_SUBTRACT:
		/* XXX: not sure */
		break;

	default:
		assert(!"unreached");
	}
}

static void
assign_lasts(struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_FLAGS:
		break;

	case AST_EXPR_ANCHOR:
		set_flags(n, AST_FLAG_LAST);
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_RANGE:
	case AST_EXPR_NAMED:
		set_flags(n, AST_FLAG_LAST);
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		set_flags(n, AST_FLAG_LAST);

		/* iterate in reverse, break on rollover */
		for (i = n->u.concat.count - 1; i < n->u.concat.count; i--) {
			struct ast_expr *child = n->u.concat.n[i];
			assign_lasts(child);
			if (always_consumes_input(child, 1) || is_end_anchor(child)) {
				break;
			}
		}
		break;
	}

	case AST_EXPR_ALT: {
		size_t i;

		set_flags(n, AST_FLAG_LAST);
		for (i = 0; i < n->u.alt.count; i++) {
			assign_lasts(n->u.alt.n[i]);
		}
		break;
	}

	case AST_EXPR_REPEATED:
		set_flags(n, AST_FLAG_LAST);
		/* Don't recurse.
		 *
		 * XXX - Not sure that this is the correct way to handle this.
		 *
		 * Recursing causes errors in the NFA when the REPEATED node is
		 * linked to the global self-loop (ie: it's either the first or
		 * last node for an unanchored RE.  Specifically, when the
		 * subexpression is compiled, the links to the global self-loop
		 * are created, which the REPEATED node then copies.
		 */
		break;

	case AST_EXPR_GROUP:
		set_flags(n, AST_FLAG_LAST);
		assign_lasts(n->u.group.e);
		break;

	case AST_EXPR_SUBTRACT:
		/* XXX: not sure */
		break;

	default:
		assert(!"unreached");
	}
}

static int
flatten(struct ast_expr *n)
{
	/* flags haven't been set yet */
	assert(n->flags == 0x0);

	switch (n->type) {
	case AST_EXPR_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			if (!flatten(n->u.concat.n[i])) {
				return 0;
			}
		}

		/* remove empty children; these have no semantic effect */
		for (i = 0; i < n->u.concat.count; ) {
			if (n->u.concat.n[i]->type == AST_EXPR_EMPTY) {
				ast_expr_free(n->u.concat.n[i]);

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

				/* because of depth-first simplification */
				assert(dead->u.concat.count >= 1);

				if (n->u.concat.alloc < n->u.concat.count + dead->u.concat.count - 1) {
					void *tmp;

					tmp = realloc(n->u.concat.n,
						(n->u.concat.count + dead->u.concat.count - 1) * sizeof *n->u.concat.n);
					if (tmp == NULL) {
						return 0;
					}

					n->u.concat.n = tmp;

					n->u.concat.alloc = (n->u.concat.count + dead->u.concat.count - 1) * sizeof *n->u.concat.n;
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
				ast_expr_free(dead);

				continue;
			}

			i++;
		}

		if (n->u.concat.count == 0) {
			free(n->u.concat.n);
			n->type = AST_EXPR_EMPTY;
			return 1;
		}

		if (n->u.concat.count == 1) {
			void *p = n->u.concat.n, *q = n->u.concat.n[0];
			*n = *n->u.concat.n[0];
			free(p);
			free(q);
			return 1;
		}

		return 1;
	}

	case AST_EXPR_ALT: {
		size_t i;

		for (i = 0; i < n->u.alt.count; i++) {
			if (!flatten(n->u.alt.n[i])) {
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

					n->u.alt.alloc = (n->u.alt.count + dead->u.alt.count - 1) * sizeof *n->u.alt.n;
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
				ast_expr_free(dead);

				continue;
			}

			i++;
		}

		/* de-duplicate children */
		if (n->u.alt.count > 1) {
			for (i = 0; i < n->u.alt.count; ) {
				if (ast_contains_expr(n->u.alt.n[i], n->u.alt.n + i + 1, n->u.alt.count - i - 1)) {
					ast_expr_free(n->u.concat.n[i]);

					if (i + 1 < n->u.concat.count) {
						memmove(&n->u.concat.n[i], &n->u.concat.n[i + 1],
							(n->u.concat.count - i - 1) * sizeof *n->u.concat.n);
					}

					n->u.concat.count--;
					continue;
				}

				i++;
			}
		}

		if (n->u.alt.count == 0) {
			free(n->u.alt.n);
			n->type = AST_EXPR_EMPTY;
			return 1;
		}

		if (n->u.alt.count == 1) {
			void *p = n->u.alt.n, *q = n->u.alt.n[0];
			*n = *n->u.alt.n[0];
			free(p);
			free(q);
			return 1;
		}

		return 1;
	}

	case AST_EXPR_GROUP:
		return flatten(n->u.group.e);

	case AST_EXPR_REPEATED:
		return flatten(n->u.repeated.e);

	default:
		return 1;
	}
}

enum ast_analysis_res
ast_analysis(struct ast *ast)
{
	enum ast_analysis_res res;

	if (ast == NULL) {
		return AST_ANALYSIS_ERROR_NULL;
	}

	assert(ast->expr != NULL);

	if (!flatten(ast->expr)) {
		return AST_ANALYSIS_ERROR_MEMORY;
	}

	/*
	 * First pass -- track nullability, clean up some artifacts from
	 * parsing, assign group IDs.
	 */
	{
		struct analysis_env env;

		memset(&env, 0x00, sizeof(env));

		res = analysis_iter(&env, ast->expr);
		if (res != AST_ANALYSIS_OK) {
			return res;
		}
	}

	/*
	 * Next passes, mark all nodes in a first and/or last position.
	 */
	{
		assign_firsts(ast->expr);
		assign_lasts(ast->expr);
	}

	/*
	 * Next pass: set anchoring, now that nullability info from
	 * the first pass is in place and some other things have been
	 * cleaned up, and note whether the regex has any unsatisfiable
	 * start anchors.
	 */
	{
		struct anchoring_env env;
		memset(&env, 0x00, sizeof(env));
		res = analysis_iter_anchoring(&env, ast->expr);
		if (res != AST_ANALYSIS_OK) { return res; }
	}

	return res;
}

