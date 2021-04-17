/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <re/re.h>

#include "class.h"
#include "print.h"
#include "ast.h"
#include "ast_analysis.h"

#define LOG_CONCAT_FLAGS 0

struct anchoring_env {
	char past_any_consuming;
	char followed_by_consuming;
	struct ast_expr_pool *pool;
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
analysis_iter(struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_EMPTY:
		set_flags(n, AST_FLAG_NULLABLE);
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
		/* no special handling */
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			if (is_nullable(n)) {
				set_flags(n->u.concat.n[i], AST_FLAG_NULLABLE);
			}
			analysis_iter(n->u.concat.n[i]);
		}

		break;
	}

	case AST_EXPR_ALT: {
		size_t i;

		for (i = 0; i < n->u.alt.count; i++) {
			analysis_iter(n->u.alt.n[i]);
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

	case AST_EXPR_REPEAT: {
		struct ast_expr *e = n->u.repeat.e;
		assert(e != NULL);

		if (n->u.repeat.min == 0) {
			set_flags(n, AST_FLAG_NULLABLE);
		}

		if (is_nullable(n)) {
			set_flags(e, AST_FLAG_NULLABLE);
		}

		analysis_iter(e);

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

		analysis_iter(e);

		if (is_nullable(e)) {
			set_flags(n, AST_FLAG_NULLABLE);
		}
		break;
	}

	case AST_EXPR_ANCHOR:
		/* anchor flags will be handled on the second pass */
		break;

	case AST_EXPR_SUBTRACT:
		/* XXX: not sure */
		break;

	case AST_EXPR_TOMBSTONE:
		set_flags(n, AST_FLAG_UNSATISFIABLE);
		return AST_ANALYSIS_UNSATISFIABLE;

	default:
		assert(!"unreached");
	}

	return AST_ANALYSIS_OK;
}

static int
always_consumes_input(const struct ast_expr *n)
{
	if (is_nullable(n)) {
		return 0;
	}

	switch (n->type) {
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
		return 1;

	case AST_EXPR_SUBTRACT:
		/*
		 * AST_EXPR_SUBTRACT nodes which produce an empty result have been
		 * replaced by AST_EXPR_EMPTY during AST rewriting. The only
		 * subtract nodes which remain here produce non-empty results,
		 * and so do always consume input.
		 */
		return 1;

	case AST_EXPR_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			if (always_consumes_input(n->u.concat.n[i])) {
				return 1;
			}
		}

		return 0;
	}

	case AST_EXPR_ALT: {
		size_t i;

		for (i = 0; i < n->u.alt.count; i++) {
			if (!always_consumes_input(n->u.alt.n[i])) {
				return 0;
			}
		}

		return 1;
	}

	case AST_EXPR_REPEAT:
		/* not nullable, so check the repeated node */
		return always_consumes_input(n->u.repeat.e);

	case AST_EXPR_ANCHOR:
		return 0;

	case AST_EXPR_GROUP:
		return always_consumes_input(n->u.group.e);

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
	case AST_EXPR_TOMBSTONE:
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
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
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

			/*
			 * If we were to encounter a tombstone here, the entire concat
			 * would be unsatisfiable because all children must be matched.
			 * However the AST rewriting resolved these already.
			 */
			assert(child->type != AST_EXPR_TOMBSTONE);

			/*
			 * Also removed during AST rewriting.
			 */
			assert(child->type != AST_EXPR_EMPTY);

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
					if (always_consumes_input(n->u.concat.n[j])) {
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
				ast_expr_free(env->pool, doomed);
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

	case AST_EXPR_REPEAT:
		res = analysis_iter_anchoring(env, n->u.repeat.e);

		/*
		 * This logic corresponds to the equivalent case for tombstone nodes
		 * during the (earlier) AST rewrite pass, except here we deal with
		 * unsatisfiable nodes rather than just tombstones.
		 *
		 * I don't like modifying the node tree here; this seems like it's
		 * being done at the wrong time. Unfortunately we need to handle the
		 * unsatisfiable nodes produced by anchors (such as /a($b)?/)
		 * and so we depend on the anchor analysis for this.
		 */
		/* TODO: maybe do the analysis before rewriting? */

		if (res == AST_ANALYSIS_UNSATISFIABLE && n->u.repeat.min == 0) {
			ast_expr_free(env->pool, n->u.repeat.e);

			n->type = AST_EXPR_EMPTY;
			set_flags(n, AST_FLAG_NULLABLE);
			break;
		}

		if (res == AST_ANALYSIS_UNSATISFIABLE && n->u.repeat.min > 0) {
			return AST_ANALYSIS_UNSATISFIABLE;
		}

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
		set_flags(n, AST_FLAG_FIRST);
		break;

	case AST_EXPR_TOMBSTONE:
		break;

	case AST_EXPR_ANCHOR:
		set_flags(n, AST_FLAG_FIRST);
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
		set_flags(n, AST_FLAG_FIRST);
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		set_flags(n, AST_FLAG_FIRST);
		for (i = 0; i < n->u.concat.count; i++) {
			struct ast_expr *child = n->u.concat.n[i];
			assign_firsts(child);

			if (always_consumes_input(child) || is_start_anchor(child)) {
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

	case AST_EXPR_REPEAT:
		set_flags(n, AST_FLAG_FIRST);
		/* Don't recurse.
		 *
		 * XXX - Not sure that this is the correct way to handle this.
		 *
		 * Recursing causes errors in the NFA when the REPEAT node is
		 * linked to the global self-loop (ie: it's either the first or
		 * last node for an unanchored RE.  Specifically, when the
		 * subexpression is compiled, the links to the global self-loop
		 * are created, which the REPEAT node then copies.
		 */
		break;

	case AST_EXPR_GROUP:
		set_flags(n, AST_FLAG_FIRST);
		assign_firsts(n->u.group.e);
		break;

	case AST_EXPR_SUBTRACT:
		set_flags(n, AST_FLAG_FIRST);
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
		set_flags(n, AST_FLAG_LAST);
		break;

	case AST_EXPR_TOMBSTONE:
		break;

	case AST_EXPR_ANCHOR:
		set_flags(n, AST_FLAG_LAST);
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
		set_flags(n, AST_FLAG_LAST);
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		set_flags(n, AST_FLAG_LAST);

		/* iterate in reverse */
		i = n->u.concat.count;
		if (i > 0) {
			do {
				i--;
				struct ast_expr *child = n->u.concat.n[i];
				assign_lasts(child);
				if (always_consumes_input(child) || is_end_anchor(child)) {
					break;
				}
			} while (i > 0);
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

	case AST_EXPR_REPEAT:
		set_flags(n, AST_FLAG_LAST);
		/* Don't recurse.
		 *
		 * XXX - Not sure that this is the correct way to handle this.
		 *
		 * Recursing causes errors in the NFA when the REPEAT node is
		 * linked to the global self-loop (ie: it's either the first or
		 * last node for an unanchored RE.  Specifically, when the
		 * subexpression is compiled, the links to the global self-loop
		 * are created, which the REPEAT node then copies.
		 */
		break;

	case AST_EXPR_GROUP:
		set_flags(n, AST_FLAG_LAST);
		assign_lasts(n->u.group.e);
		break;

	case AST_EXPR_SUBTRACT:
		set_flags(n, AST_FLAG_LAST);
		/* XXX: not sure */
		break;

	default:
		assert(!"unreached");
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

	/*
	 * First pass -- track nullability, clean up some artifacts from
	 * parsing.
	 */
	res = analysis_iter(ast->expr);
	if (res != AST_ANALYSIS_OK) {
		return res;
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
		env.pool = ast->pool;
		res = analysis_iter_anchoring(&env, ast->expr);
		if (res != AST_ANALYSIS_OK) { return res; }
	}

	return res;
}

