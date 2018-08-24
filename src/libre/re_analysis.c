/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "re_analysis.h"
#include "print.h"

#include <string.h>

#define LOG_CONCAT_FLAGS 0

struct analysis_env {
	unsigned group_id;
};

struct anchoring_env {
	char past_any_consuming;
	char followed_by_consuming;
};

static int
flatten(struct ast_expr **n);

static enum re_analysis_res
analysis_iter(struct analysis_env *env, struct ast_expr *n);

static enum re_analysis_res
analysis_iter_anchoring(struct anchoring_env *env, struct ast_expr *n);

static int
is_nullable(const struct ast_expr *n) { return n->flags & RE_AST_FLAG_NULLABLE; }

static void
set_flags(struct ast_expr *n, enum re_ast_flags f);

static int
always_consumes_input(const struct ast_expr *n, int thud);

static void
assign_firsts(struct ast_expr *n);
static void
assign_lasts(struct ast_expr *n);

enum re_analysis_res
re_ast_analysis(struct ast_re *ast)
{
	enum re_analysis_res res;

	if (ast == NULL) { return RE_ANALYSIS_ERROR_NULL; }

	assert(ast->expr != NULL);

	if (!flatten(&ast->expr)) {
		return RE_ANALYSIS_ERROR_MEMORY;
	}

	/* First pass -- track nullability, clean up some artifacts from
	 * parsing, assign group IDs. */
        {
		struct analysis_env env;
		memset(&env, 0x00, sizeof(env));
		res = analysis_iter(&env, ast->expr);
	}
	if (res != RE_ANALYSIS_OK) {
		if (res == RE_ANALYSIS_UNSATISFIABLE) {
			ast->unsatisfiable = 1;
		}
		return res;
	}

	/* Next passes, mark all nodes in a
	 * first and/or last position. */
        {
		assign_firsts(ast->expr);
		assign_lasts(ast->expr);
	}

	/* Next pass: set anchoring, now that nullability info from
	 * the first pass is in place and some other things have been
	 * cleaned up, and note whether the regex has any unsatisfiable
	 * start anchors.*/
        {
		struct anchoring_env env;
		memset(&env, 0x00, sizeof(env));
		res = analysis_iter_anchoring(&env, ast->expr);
		if (res != RE_ANALYSIS_OK) { return res; }
	}

	return res;
}

static unsigned
count_chain(const struct ast_expr *n, enum ast_expr_type t)
{
	unsigned res = 0;
	for (;;) {
		assert(n != NULL);
		if (n->t == AST_EXPR_EMPTY) {
			return res;
		} else {
			assert(n->t == t);
			res++;
			switch (t) {
			case AST_EXPR_CONCAT:
				n = n->u.concat.r;
				break;
			case AST_EXPR_ALT:
				n = n->u.alt.r;
				break;
			default:
				assert(0);
				break;
			}
		}
	}
}

static struct ast_expr *
collect_chain(size_t count, struct ast_expr *doomed)
{
	size_t i;
	if (doomed->t == AST_EXPR_CONCAT) {
		if (count == 1) {
			struct ast_expr *res = doomed->u.concat.l;
			if (!flatten(&doomed->u.concat.l)) { return 0; }
			res = doomed->u.concat.l;

			assert(doomed->u.concat.r->t == AST_EXPR_EMPTY);
			doomed->u.concat.l = re_ast_expr_tombstone();
			re_ast_expr_free(doomed);
			assert(res->t != AST_EXPR_CONCAT);
			return res;
		} else {
			struct ast_expr *dst = re_ast_expr_concat_n(count);
			if (dst == NULL) { return NULL; }
			
			for (i = 0; i < count; i++) {
				struct ast_expr *ndoomed = doomed->u.concat.r;
				if (!flatten(&doomed->u.concat.l)) { return 0; }
				dst->u.concat_n.n[i] = doomed->u.concat.l;
				doomed->u.concat.l = re_ast_expr_tombstone();
				doomed->u.concat.r = re_ast_expr_tombstone();
				re_ast_expr_free(doomed);
				doomed = ndoomed;
				if (i == count - 1) {
					assert(doomed->t == AST_EXPR_EMPTY);
				}
			}
			return dst;
		}
	} else if (doomed->t == AST_EXPR_ALT) {
		if (count == 1) {
			/* If we get here, it's a parser bug. Right? */
			assert(0);
		} else {
			struct ast_expr *dst = re_ast_expr_alt_n(count);
			if (dst == NULL) { return NULL; }

			for (i = 0; i < count; i++) {
				struct ast_expr *ndoomed = doomed->u.alt.r;
				if (!flatten(&doomed->u.alt.l)) { return 0; }
				dst->u.alt_n.n[i] = doomed->u.alt.l;
				doomed->u.alt.l = re_ast_expr_tombstone();
				doomed->u.alt.r = re_ast_expr_tombstone();
				re_ast_expr_free(doomed);
				doomed = ndoomed;
				if (i == count - 1) {
					assert(doomed->t == AST_EXPR_EMPTY);
				}
			}
			return dst;
		}
	} else {
		assert(0);
		return NULL;
	}
}

/* TODO: do this directly in the parser. */
static struct ast_expr *
flatten_iter(struct ast_expr *n)
{
	switch (n->t) {
	default:
		return n;

	case AST_EXPR_CONCAT:
	{
		const unsigned count = count_chain(n, AST_EXPR_CONCAT);
		return collect_chain(count, n);
	}

	case AST_EXPR_ALT:
	{
		const unsigned count = count_chain(n, AST_EXPR_ALT);
		return collect_chain(count, n);
	}

	case AST_EXPR_GROUP:
		if (!flatten(&n->u.group.e)) { return NULL; }
		return n;

	case AST_EXPR_REPEATED:
		if (!flatten(&n->u.repeated.e)) { return NULL; }
		return n;
	}
}

static int
flatten(struct ast_expr **n)
{
	struct ast_expr *res;
	res = flatten_iter(*n);
	if (res == NULL) {
		return 0;
	} else {
		*n = res;
		return 1;
	}
}

static enum re_analysis_res
analysis_iter(struct analysis_env *env, struct ast_expr *n)
{
	switch (n->t) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
		/* no special handling */
		break;

	case AST_EXPR_CONCAT_N:
	{
		size_t i;
		for (i = 0; i < n->u.concat_n.count; i++) {
			if (is_nullable(n)) {
				set_flags(n->u.concat_n.n[i], RE_AST_FLAG_NULLABLE);
			}
			analysis_iter(env, n->u.concat_n.n[i]);
		}
		break;
	}

	case AST_EXPR_ALT_N:
	{
		size_t i;
		for (i = 0; i < n->u.alt_n.count; i++) {
			analysis_iter(env, n->u.alt_n.n[i]);
			/* spread nullability upward */
			if (is_nullable(n->u.alt_n.n[i])) {
				set_flags(n, RE_AST_FLAG_NULLABLE);
			}
		}

		if (is_nullable(n)) { /* spread nullability downward */
			for (i = 0; i < n->u.alt_n.count; i++) {
				set_flags(n->u.alt_n.n[i], RE_AST_FLAG_NULLABLE);
			}
		}
		break;
	}

	case AST_EXPR_REPEATED:
	{
		struct ast_expr *e = n->u.repeated.e;
		assert(e != NULL);

		if (n->u.repeated.low == 0) {
			set_flags(n, RE_AST_FLAG_NULLABLE);
		}

		if (is_nullable(n)) {
			set_flags(e, RE_AST_FLAG_NULLABLE);
		}

		analysis_iter(env, e);

		if (is_nullable(e)) { set_flags(n, RE_AST_FLAG_NULLABLE); }
		break;
	}

	case AST_EXPR_GROUP:
	{
		struct ast_expr *e = n->u.group.e;
		if (is_nullable(n)) { set_flags(e, RE_AST_FLAG_NULLABLE); }

		/* assign group ID */
		env->group_id++;
		n->u.group.id = env->group_id;

		analysis_iter(env, e);

		if (is_nullable(e)) { set_flags(n, RE_AST_FLAG_NULLABLE); }
		break;
	}

	case AST_EXPR_FLAGS:
		set_flags(n, RE_AST_FLAG_NULLABLE); 
		break;

	case AST_EXPR_CHAR_CLASS:
		/* character classes cannot be empty, so they're not nullable */
		break;

	case AST_EXPR_ANCHOR:
		/* anchor flags will be handled on the second pass */
		break;

	default:
		fprintf(stderr, "%s:%d: <matchfail %d>\n",
		    __FILE__, __LINE__, n->t);
		abort();
	}

	return RE_ANALYSIS_OK;
}

static int
always_consumes_input(const struct ast_expr *n, int thud)
{
	if (is_nullable(n)) { return 0; }

	switch (n->t) {
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_CHAR_CLASS:
		return 1;

	case AST_EXPR_CONCAT_N:
	{
		size_t i;	/* check if any consume input */
		for (i = 0; i < n->u.concat_n.count; i++) {
			if (always_consumes_input(n->u.concat_n.n[i], thud)) {
				return 1;
			}
		}
		return 0;
	}

	case AST_EXPR_ALT_N:
	{
		size_t i;	/* check if all consume input */
		for (i = 0; i < n->u.alt_n.count; i++) {
			if (!always_consumes_input(n->u.alt_n.n[i], thud)) {
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
		if (thud) { return 1; } /* FIXME */
		return always_consumes_input(n->u.group.e, thud);

	default:
		return 0;
	}
}

static int
is_start_anchor(const struct ast_expr *n) {
	return n->t == AST_EXPR_ANCHOR && n->u.anchor.t == RE_AST_ANCHOR_START;
}

static int
is_end_anchor(const struct ast_expr *n) {
	return n->t == AST_EXPR_ANCHOR && n->u.anchor.t == RE_AST_ANCHOR_END;
}

static enum re_analysis_res
analysis_iter_anchoring(struct anchoring_env *env, struct ast_expr *n)
{
	enum re_analysis_res res;
	/* Flag the overall RE as unsatisfiable if there are anchors
	 * that cannot be simultaneously satisfied. */
	
	switch (n->t) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_FLAGS:
		break;

	case AST_EXPR_ANCHOR:
		if (n->u.anchor.t == RE_AST_ANCHOR_START) {
			/* If it's not possible to get here without consuming
			 * any input and there's a start anchor, the regex is
			 * inherently unsatisfiable. */

			if (env->past_any_consuming) {
				set_flags(n, RE_AST_FLAG_UNSATISFIABLE);
				/* Start anchors that are not in a FIRST position
				 * almost perfectly match unsatisfiable start
				 * anchors, with the exception of multiple start
				 * anchors in a row (e.g. `^^ab$`). We can't just
				 * treat them as nullable either, because it messes
				 * up the results for linking. */
				assert(0 == (n->flags & RE_AST_FLAG_FIRST_STATE));
				return RE_ANALYSIS_UNSATISFIABLE;
			}
		} else if (n->u.anchor.t == RE_AST_ANCHOR_END) {
			if (env->followed_by_consuming) {
				/* See above: This is a near perfect subset of
				 * unsatisfiable cases, but fails to handle
				 * redundant $ anchors correctly (including
				 * ones with arbitrary nesting). */
				assert(0 == (n->flags & RE_AST_FLAG_LAST_STATE));
				set_flags(n, RE_AST_FLAG_UNSATISFIABLE);
				return RE_ANALYSIS_UNSATISFIABLE;
			}
		} else {
			assert(0);
		}
		break;

	/* These are the types that actually consume input. */
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_CHAR_CLASS:
		if (!is_nullable(n)) {
			env->past_any_consuming = 1;
		}
		break;

	case AST_EXPR_CONCAT_N:
	{
		size_t i, j;
		char bak_consuming_after = 0;

		for (i = 0; i < n->u.concat_n.count; i++) {
			struct ast_expr *child = n->u.concat_n.n[i];
#if LOG_CONCAT_FLAGS
			fprintf(stderr, "%s: %p: %lu: %p -- past_any %d\n",
			    __func__, (void *)n, i, (void *)child,
			    env->past_any_consuming);
#endif

			/* Before recursing, save and restore whether
			 * this is followed by anything which always consumes input.
			 *
			 * TODO: instead of doing a backward sweep for each node,
			 * do a reverse falsification step as a separate pass? */
			bak_consuming_after = env->followed_by_consuming;

			/* If we don't already know whether this is being followed
			 * by something that always consumes input, check */
			if (!env->followed_by_consuming) {
				for (j = i + 1; j < n->u.concat_n.count; j++) {
					if (always_consumes_input(n->u.concat_n.n[j], 0)) {
						env->followed_by_consuming = 1;
						break;
					}
				}
			}

			res = analysis_iter_anchoring(env, child);

			env->followed_by_consuming = bak_consuming_after;

			if (res != RE_ANALYSIS_OK) { return res; }
		}

		break;
	}

	case AST_EXPR_ALT_N:
	{
		size_t i;
		int any_sat = 0;

		for (i = 0; i < n->u.alt_n.count; i++) {
			struct anchoring_env bak;
			memcpy(&bak, env, sizeof(*env));

			res = analysis_iter_anchoring(env, n->u.alt_n.n[i]);

			if (res == RE_ANALYSIS_UNSATISFIABLE) {
				/* prune unsatisfiable branch */
				struct ast_expr *doomed = n->u.alt_n.n[i];
				n->u.alt_n.n[i] = re_ast_expr_tombstone();
				re_ast_expr_free(doomed);
				continue;
			} else if (res == RE_ANALYSIS_OK) {
				any_sat = 1;
			} else {
				return res;
			}

			/* restore */
			memcpy(env, &bak, sizeof(*env));
		}

		/* An ALT group is only unstaisfiable if they ALL are. */
		if (!any_sat) {
			set_flags(n, RE_AST_FLAG_UNSATISFIABLE);
			return RE_ANALYSIS_UNSATISFIABLE;
		}
		break;
	}

	case AST_EXPR_REPEATED:
		res = analysis_iter_anchoring(env, n->u.repeated.e);
		if (res != RE_ANALYSIS_OK) { return res; }
		break;

	case AST_EXPR_GROUP:
		res = analysis_iter_anchoring(env, n->u.group.e);
		if (res != RE_ANALYSIS_OK) { return res; }
		break;

	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
		break;
	    }

	return RE_ANALYSIS_OK;
}

static void
assign_firsts(struct ast_expr *n)
{
	switch (n->t) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_FLAGS:
		break;

	case AST_EXPR_ANCHOR:
		if (n->u.anchor.t == RE_AST_ANCHOR_START) {
			set_flags(n, RE_AST_FLAG_FIRST_STATE);
		}
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_CHAR_CLASS:
		set_flags(n, RE_AST_FLAG_FIRST_STATE);
		break;

	case AST_EXPR_CONCAT_N:
	{
		size_t i;
		set_flags(n, RE_AST_FLAG_FIRST_STATE);
		for (i = 0; i < n->u.concat_n.count; i++) {
			struct ast_expr *child = n->u.concat_n.n[i];
			assign_firsts(child);

			if (always_consumes_input(child, 1) || is_start_anchor(child)) { break; }
		}
		break;
	}

	case AST_EXPR_ALT_N:
	{
		size_t i;
		set_flags(n, RE_AST_FLAG_FIRST_STATE);
		for (i = 0; i < n->u.alt_n.count; i++) {
			assign_firsts(n->u.alt_n.n[i]);
		}
		break;
	}

	case AST_EXPR_REPEATED:
		set_flags(n, RE_AST_FLAG_FIRST_STATE);
		assign_firsts(n->u.repeated.e);
		break;

	case AST_EXPR_GROUP:
		set_flags(n, RE_AST_FLAG_FIRST_STATE);
		assign_firsts(n->u.group.e);
		break;

	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
		break;
	    }
}

static void
assign_lasts(struct ast_expr *n) {
	switch (n->t) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_FLAGS:
		break;

	case AST_EXPR_ANCHOR:
		set_flags(n, RE_AST_FLAG_LAST_STATE);
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_CHAR_CLASS:
		set_flags(n, RE_AST_FLAG_LAST_STATE);
		break;

	case AST_EXPR_CONCAT_N:
	{
		size_t i;
		set_flags(n, RE_AST_FLAG_LAST_STATE);

		/* iterate in reverse, break on rollover */
		for (i = n->u.concat_n.count - 1; i < n->u.concat_n.count; i--) {
			struct ast_expr *child = n->u.concat_n.n[i];
			assign_lasts(child);
			if (always_consumes_input(child, 1) || is_end_anchor(child)) { break; }
		}
		break;
	}

	case AST_EXPR_ALT_N:
	{
		size_t i;
		set_flags(n, RE_AST_FLAG_LAST_STATE);
		for (i = 0; i < n->u.alt_n.count; i++) {
			assign_lasts(n->u.alt_n.n[i]);
		}
		break;
	}

	case AST_EXPR_REPEATED:
		set_flags(n, RE_AST_FLAG_LAST_STATE);
		assign_lasts(n->u.repeated.e);
		break;

	case AST_EXPR_GROUP:
		set_flags(n, RE_AST_FLAG_LAST_STATE);
		assign_lasts(n->u.group.e);
		break;

	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
		break;
	    }
}

static void
set_flags(struct ast_expr *n, enum re_ast_flags f)
{
	n->flags |= f;
}
