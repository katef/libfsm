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

#include <adt/common.h>

#define LOG_ANALYSIS 0
#define LOG_FIRST_ANALYSIS (0 + LOG_ANALYSIS)
#define LOG_REPEATED_GROUPS (0 + LOG_ANALYSIS)
#define LOG_FORKING (0 + LOG_ANALYSIS)
#define LOG_ANCHORING (0 + LOG_ANALYSIS)
#define LOG_CONCAT_FLAGS (0 + LOG_ANALYSIS)
#define LOG_UNANCHORED_FLAGS (0 + LOG_ANALYSIS)

#define LOG_REPETITION_CASES (0 + LOG_ANALYSIS)
#define LOG_PINCER_ANCHORS (0 + LOG_ANALYSIS)

#define LOG(LEVEL, ...)							\
	do {								\
		if ((LEVEL) <= LOG_ANALYSIS) {				\
			fprintf(stderr, __VA_ARGS__);			\
		}							\
	} while(0)

/* Mask for end-anchor flags */
#define END_ANCHOR_FLAG_MASK (AST_FLAG_ANCHORED_END | AST_FLAG_END_NL)
#define ANCHOR_FLAG_MASK (END_ANCHOR_FLAG_MASK | AST_FLAG_ANCHORED_START)

static int
is_nullable(const struct ast_expr *n)
{
	if (n->type == AST_EXPR_EMPTY) {
		return 1;
	}

	return n->flags & AST_FLAG_NULLABLE;
}

static int
can_consume_input(const struct ast_expr *n)
{
	/* Hacky special case. This is probably preferable to
	 * setting/clearing the AST_FLAG_CAN_CONSUME flag. */
	if (n->type == AST_EXPR_REPEAT && n->u.repeat.max == 0) {
		return 0;
	}

	return n->flags & AST_FLAG_CAN_CONSUME;
}

static int
always_consumes_input(const struct ast_expr *n)
{
	if (is_nullable(n)) { return 0; }
	if (!can_consume_input(n)) { return 0; }
	return n->flags & AST_FLAG_ALWAYS_CONSUMES;
}

static void
set_flags(struct ast_expr *n, enum ast_flags flags)
{
	n->flags |= flags;
}

static int
is_unsatisfiable_or_skipped(const struct ast_expr *n)
{
	return (n->flags & AST_FLAG_UNSATISFIABLE)
	    || (n->type == AST_EXPR_REPEAT
		&& n->u.repeat.min == 0
		&& n->u.repeat.max == 0);
}

static void
prune_repeat_node(struct ast_expr *n)
{
	assert(n->type == AST_EXPR_REPEAT && n->u.repeat.min == 0);
	LOG(3, "%s: setting REPEAT node %p's max count to 0\n",
	    __func__, (void *)n);
	n->u.repeat.max = 0;
}

static enum ast_analysis_res
analysis_iter(struct ast_expr *n)
{
	assert(n != NULL);
	LOG(3 - LOG_FIRST_ANALYSIS, "%s: n %p, type %s\n",
	    __func__, (void *)n, ast_node_type_name(n->type));

	switch (n->type) {
	case AST_EXPR_EMPTY:
		set_flags(n, AST_FLAG_NULLABLE);
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
		set_flags(n, AST_FLAG_CAN_CONSUME | AST_FLAG_ALWAYS_CONSUMES);
		break;

	case AST_EXPR_CONCAT: {
		size_t i;
		int any_can_consume = 0;
		int any_always_consumes = 0;
		int all_nullable = 1;

		for (i = 0; i < n->u.concat.count; i++) {
			struct ast_expr *child = n->u.concat.n[i];
			if (is_nullable(n)) {
				LOG(3 - LOG_FIRST_ANALYSIS, "%p: setting child %p nullable\n",
				    (void *)n, (void *)child);
				set_flags(child, AST_FLAG_NULLABLE);
			}
			enum ast_analysis_res child_res = analysis_iter(child);
                        if (child_res != AST_ANALYSIS_OK) { return child_res; }

			if (can_consume_input(child)) {
				any_can_consume = 1;
			}
			if (always_consumes_input(child)) {
				any_always_consumes = 1;
			}

			if (!is_nullable(child)) {
				all_nullable = 0;
			}
		}

		if (any_can_consume) {
			LOG(3 - LOG_FIRST_ANALYSIS, "%p: any_can_consume -> setting CAN_CONSUME\n",
			    (void *)n);
			set_flags(n, AST_FLAG_CAN_CONSUME);
		}

		if (any_always_consumes) {
			LOG(3 - LOG_FIRST_ANALYSIS, "%p: any_always_consumes -> setting ALWAYS_CONSUMES\n",
			    (void *)n);
			assert(any_can_consume);
			set_flags(n, AST_FLAG_ALWAYS_CONSUMES);
		}
		if (n->u.concat.count > 0 && all_nullable) {
			set_flags(n, AST_FLAG_NULLABLE);
		}

		break;
	}

	case AST_EXPR_ALT: {
		size_t i;
		int any_can_consume = 0;
		size_t always_consumes_count = 0;

		for (i = 0; i < n->u.alt.count; i++) {
			struct ast_expr *child = n->u.alt.n[i];
			enum ast_analysis_res child_res = analysis_iter(child);
                        if (child_res != AST_ANALYSIS_OK) { return child_res; }

			/* spread nullability upward */
			if (is_nullable(child)) {
				LOG(3 - LOG_FIRST_ANALYSIS, "%p: child %p is nullable, spreading upward\n",
				    (void *)child, (void *)n);
				set_flags(n, AST_FLAG_NULLABLE);
			}

			if (can_consume_input(child)) {
				any_can_consume = 1;
			}
			if (always_consumes_input(child)) {
				always_consumes_count++;
			}
		}

		if (any_can_consume) {
			LOG(3 - LOG_FIRST_ANALYSIS,
			    "%p: any_can_consume -> setting CAN_CONSUME\n", (void *)n);
			set_flags(n, AST_FLAG_CAN_CONSUME);
		}

		/* FIXME: This should not be invalidated by unsatisfiable nodes without the
		 * AST_FLAG_ALWAYS_CONSUMES flag set, but since unsatisfiability derives
		 * from needing to consume input in a way that contradicts anchoring, it
		 * may not be possible to produce a case where an alt subtree is
		 * unsatisfiable but does not set AST_FLAG_ALWAYS_CONSUMES. */
		if (always_consumes_count == n->u.alt.count) {
			LOG(3 - LOG_FIRST_ANALYSIS,
			    "%p: all always_consume -> setting ALWAYS_CONSUMES\n", (void *)n);
			set_flags(n, AST_FLAG_ALWAYS_CONSUMES);
		}

		break;
	}

	case AST_EXPR_REPEAT: {
		struct ast_expr *e = n->u.repeat.e;
		assert(e != NULL);

		if (n->u.repeat.min == 0) {
			LOG(3 - LOG_FIRST_ANALYSIS,
			    "%p: repeat.min is 0 -> setting NULLABLE\n", (void *)n);
			set_flags(n, AST_FLAG_NULLABLE);
		}

                enum ast_analysis_res child_res = analysis_iter(e);
                if (child_res != AST_ANALYSIS_OK) { return child_res; }
		set_flags(n, e->flags & AST_FLAG_CAN_CONSUME);

		if (n->u.repeat.min > 0 && e->flags & AST_FLAG_ALWAYS_CONSUMES) {
			LOG(3 - LOG_FIRST_ANALYSIS,
			    "%p: setting ALWAYS_CONSUMES because child %p does\n",
			    (void *)n, (void *)e);
			set_flags(n, AST_FLAG_ALWAYS_CONSUMES);
		}

		if (is_nullable(e)) {
			LOG(3 - LOG_FIRST_ANALYSIS,
			    "%p: spreading nullability upward from child %p\n",
			    (void *)n, (void *)e);
			set_flags(n, AST_FLAG_NULLABLE);
		}

		/* When building for fuzzing, ignore uninteresting
		 * failures from regexes like '.{1000000}' that use
		 * repetition to hit memory limits. */
#if BUILD_FOR_FUZZER
                if ((n->u.repeat.max != AST_COUNT_UNBOUNDED && n->u.repeat.max >= 10)
		    || (n->u.repeat.min >= 10)) {
			fprintf(stderr, "%s: rejecting regex with {count} >= 10 (%u)\n",
			    __func__, n->u.repeat.max);
			return AST_ANALYSIS_ERROR_MEMORY;
                }
#endif

		break;
	}

	case AST_EXPR_GROUP: {
		struct ast_expr *e = n->u.group.e;

		if (is_nullable(n)) {
			set_flags(e, AST_FLAG_NULLABLE);
		}

                enum ast_analysis_res child_res = analysis_iter(e);
                if (child_res != AST_ANALYSIS_OK) { return child_res; }
		set_flags(n, e->flags & (AST_FLAG_CAN_CONSUME | AST_FLAG_ALWAYS_CONSUMES));

		if (is_nullable(e)) {
			set_flags(n, AST_FLAG_NULLABLE);
		}
		break;
	}

	case AST_EXPR_ANCHOR:
		/* anchor flags will be handled on the second pass */
		break;

	case AST_EXPR_SUBTRACT:
	{
		enum ast_analysis_res res = analysis_iter(n->u.subtract.a);
		if (res != AST_ANALYSIS_OK) { return res; }
		res = analysis_iter(n->u.subtract.b);
		if (res != AST_ANALYSIS_OK) { return res; }
		enum ast_flags a_consume_flags = n->u.subtract.a->flags &
		    (AST_FLAG_CAN_CONSUME | AST_FLAG_ALWAYS_CONSUMES);
		if (a_consume_flags != 0) {
			set_flags(n, a_consume_flags);
		}
		break;
	}

	case AST_EXPR_TOMBSTONE:
		set_flags(n, AST_FLAG_UNSATISFIABLE);
		return AST_ANALYSIS_UNSATISFIABLE;

	default:
		assert(!"unreached");
	}

	return AST_ANALYSIS_OK;
}

/* Analysis for unanchored starts/ends uses three values, because some
 * details decide the whole subtree is/isn't, others should defer to
 * analysis elsewhere it the tree, but an overall result of undecided
 * still decides yes. */
enum unanchored_analysis_res {
	UA_NO = 'N',
	UA_YES = 'Y',
	UA_UNDECIDED = 'U',
};

static enum unanchored_analysis_res
analysis_iter_unanchored_start(const struct ast_expr *n)
{
	if (n->flags & AST_FLAG_ANCHORED_START) {
		LOG(4 - LOG_UNANCHORED_FLAGS, "%s: n (%p)->flags & AST_FLAG_ANCHORED_START -> N\n",
		    __func__, (void *)n);
		return UA_NO;
	}

	LOG(4 - LOG_UNANCHORED_FLAGS, "%s: node %p, type %s\n",
	    __func__, (void *)n, ast_node_type_name(n->type));

	switch (n->type) {
	case AST_EXPR_EMPTY:
		LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> U\n",
		    __func__, ast_node_type_name(n->type));
		return UA_UNDECIDED;

	case AST_EXPR_TOMBSTONE:
		break;

	case AST_EXPR_ANCHOR:
		switch (n->u.anchor.type) {
		case AST_ANCHOR_START:
			LOG(3 - LOG_UNANCHORED_FLAGS, "%s: ^ anchor -> N\n", __func__);
			return UA_NO;

		case AST_ANCHOR_END:
			LOG(3 - LOG_UNANCHORED_FLAGS, "%s: $ anchor -> U\n", __func__);
			/* will be handled by other cases */
			return UA_UNDECIDED;

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
		LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> Y\n",
		    __func__, ast_node_type_name(n->type));
		return UA_YES;

	case AST_EXPR_CONCAT: {
		size_t i;

		/* has unanchored start if first non-nullable child does */
		for (i = 0; i < n->u.concat.count; i++) {
			const struct ast_expr *child = n->u.concat.n[i];
			const enum unanchored_analysis_res child_res = analysis_iter_unanchored_start(child);
			if (child_res != UA_UNDECIDED) {
				LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> %c (child res)\n",
				    __func__, ast_node_type_name(n->type), child_res);
				return child_res;
			}

			if (always_consumes_input(child)) {
				break;
			}
		}

		break;
	}

	case AST_EXPR_ALT: {
		size_t i;

		/* if all children agree, return that result, otherwise undecided */
		const enum unanchored_analysis_res first_child_res = analysis_iter_unanchored_start(n->u.alt.n[0]);
		LOG(3 - LOG_UNANCHORED_FLAGS, "%s: ALT child 0 -- %s -> %c (child res)\n",
		    __func__, ast_node_type_name(n->type), first_child_res);

		for (i = 1; i < n->u.alt.count; i++) {
			const struct ast_expr *child = n->u.alt.n[i];
			const enum unanchored_analysis_res child_res = analysis_iter_unanchored_start(child);
			LOG(3 - LOG_UNANCHORED_FLAGS, "%s: ALT child %zd -- %s -> %c (child res)\n",
			    __func__, i, ast_node_type_name(n->type), child_res);
			if (child_res != first_child_res) {
				LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> %c (child result) contracts first, returning U\n",
				    __func__, ast_node_type_name(n->type), child_res);
				return UA_UNDECIDED;
			}
		}

		LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> %c (all children agree)\n",
		    __func__, ast_node_type_name(n->type), first_child_res);
		return first_child_res;
	}

	case AST_EXPR_REPEAT:
		if (n->u.repeat.min == 0) {
			LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> U (repeat.min == 0)\n",
			    __func__, ast_node_type_name(n->type));
			return UA_UNDECIDED;
		}
		return analysis_iter_unanchored_start(n->u.repeat.e);

	case AST_EXPR_GROUP:
		return analysis_iter_unanchored_start(n->u.group.e);

	case AST_EXPR_SUBTRACT:
		return analysis_iter_unanchored_start(n->u.subtract.a);

	default:
		assert(!"unreached");
	}

	return UA_UNDECIDED;
}

static enum unanchored_analysis_res
analysis_iter_unanchored_end(const struct ast_expr *n)
{
	if (n->flags & AST_FLAG_ANCHORED_END) {
		LOG(4 - LOG_UNANCHORED_FLAGS, "%s: node (%p)->flags & AST_FLAG_ANCHORED_END -> N\n",
		    __func__, (void *)n);
		return UA_NO;
	}

	LOG(4 - LOG_UNANCHORED_FLAGS, "%s: node %p, type %s\n",
	    __func__, (void *)n, ast_node_type_name(n->type));

	switch (n->type) {
	case AST_EXPR_EMPTY:
		LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> U\n",
		    __func__, ast_node_type_name(n->type));
		return UA_UNDECIDED;

	case AST_EXPR_TOMBSTONE:
		break;

	case AST_EXPR_ANCHOR:
		switch (n->u.anchor.type) {
		case AST_ANCHOR_START:
			LOG(3 - LOG_UNANCHORED_FLAGS, "%s: ^ %s -> U\n",
			    __func__, ast_node_type_name(n->type));
			return UA_UNDECIDED;

		case AST_ANCHOR_END:
			LOG(3 - LOG_UNANCHORED_FLAGS, "%s: $ %s -> N\n",
			    __func__, ast_node_type_name(n->type));
			return UA_NO;

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
		LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> Y\n",
		    __func__, ast_node_type_name(n->type));
		return UA_YES;

	case AST_EXPR_CONCAT: {
		size_t i;

		/* has unanchored end if last non-nullable child does */
		for (i = n->u.concat.count; i > 0; i--) {
			const struct ast_expr *child = n->u.concat.n[i - 1];
			const enum unanchored_analysis_res child_res = analysis_iter_unanchored_end(child);
			if (child_res != UA_UNDECIDED) {
				LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> %c (child res)\n",
				    __func__, ast_node_type_name(n->type), child_res);
				return child_res;
			}

			if (!is_nullable(child)) {
				break;
			}
		}

		break;
	}

	case AST_EXPR_ALT: {
		size_t i;

		/* if all children agree, return that result, otherwise undecided */
		const enum unanchored_analysis_res first_child_res = analysis_iter_unanchored_end(n->u.alt.n[0]);
		LOG(3 - LOG_UNANCHORED_FLAGS, "%s: ALT child 0 -- %s -> %c (child res)\n",
		    __func__, ast_node_type_name(n->type), first_child_res);

		for (i = 1; i < n->u.alt.count; i++) {
			const struct ast_expr *child = n->u.alt.n[i];
			const enum unanchored_analysis_res child_res = analysis_iter_unanchored_end(child);
			LOG(3 - LOG_UNANCHORED_FLAGS, "%s: ALT child %zd -- %s -> %c (child res)\n",
			    __func__, i, ast_node_type_name(n->type), child_res);
			if (child_res != first_child_res) {
				LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> %c (child result) contracts first, returning U\n",
				    __func__, ast_node_type_name(n->type), child_res);
				return UA_UNDECIDED;
			}
		}

		LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> %c (all children agree)\n",
		    __func__, ast_node_type_name(n->type), first_child_res);
		return first_child_res;
	}

	case AST_EXPR_REPEAT:
		if (n->u.repeat.min == 0) {
			LOG(3 - LOG_UNANCHORED_FLAGS, "%s: %s -> U (repeat.min == 0)\n",
			    __func__, ast_node_type_name(n->type));
			return UA_UNDECIDED;
		}
		return analysis_iter_unanchored_end(n->u.repeat.e);

	case AST_EXPR_GROUP:
		return analysis_iter_unanchored_end(n->u.group.e);

	case AST_EXPR_SUBTRACT:
		return analysis_iter_unanchored_end(n->u.subtract.a);

	default:
		assert(!"unreached");
	}

	return UA_UNDECIDED;
}

static void
set_flags_subtree(struct ast_expr *n, enum ast_flags flags)
{
	n->flags |= flags;

	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_TOMBSTONE:
	case AST_EXPR_ANCHOR:
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
		break;

	case AST_EXPR_CONCAT: {
		for (size_t i = 0; i < n->u.concat.count; i++) {
			struct ast_expr *child = n->u.concat.n[i];
			set_flags_subtree(child, flags);
		}
		break;
	}

	case AST_EXPR_ALT: {
		for (size_t i = 0; i < n->u.alt.count; i++) {
			struct ast_expr *child = n->u.alt.n[i];
			set_flags_subtree(child, flags);
		}
		break;
	}

	case AST_EXPR_REPEAT:
		set_flags_subtree(n->u.repeat.e, flags);
		break;

	case AST_EXPR_GROUP:
		set_flags_subtree(n->u.group.e, flags);
		break;

	case AST_EXPR_SUBTRACT:
		set_flags_subtree(n->u.subtract.a, flags);
		break;
	}
}

static int
can_consume_single_newline(struct ast_expr *n)
{
	if (!can_consume_input(n)) { return 0; }

	if (n->flags & AST_FLAG_MATCHES_1NEWLINE) { return 1; }

	switch (n->type) {
	case AST_EXPR_LITERAL:
		return n->u.literal.c == '\n';

	case AST_EXPR_CODEPOINT:
		return n->u.codepoint.u == (uint32_t)'\n';

	case AST_EXPR_RANGE:
		if ((n->u.range.from.type == AST_ENDPOINT_LITERAL) &&
		    (n->u.range.to.type == AST_ENDPOINT_LITERAL)) {
			return n->u.range.from.u.literal.c <= '\n'
			    && n->u.range.to.u.literal.c >= '\n';
		} else if ((n->u.range.from.type == AST_ENDPOINT_CODEPOINT) &&
		    (n->u.range.to.type == AST_ENDPOINT_CODEPOINT)) {
			    return n->u.range.from.u.codepoint.u <= '\n'
				&& n->u.range.to.u.codepoint.u >= '\n';
		} else if (n->u.range.from.type == AST_ENDPOINT_NAMED) {
			/* currently unreachable, named ranges are unsupported */
			assert(!"unreachable");
			break;
		}
		break;

	case AST_EXPR_SUBTRACT:
		return can_consume_single_newline(n->u.subtract.a)
		    && !can_consume_single_newline(n->u.subtract.b);

	default:
		break;
	}

	return 0;
}

/* Does the subtree match a literal '\n'? */
static int
matches_newline(const struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_LITERAL:
		return n->u.literal.c == '\n';

	case AST_EXPR_SUBTRACT:
		return matches_newline(n->u.subtract.a)
		    && !matches_newline(n->u.subtract.b);

	case AST_EXPR_RANGE:
	{
		const struct ast_endpoint *f = &n->u.range.from;
		const struct ast_endpoint *t = &n->u.range.to;
		if (f->type != AST_ENDPOINT_LITERAL
		    || t->type != AST_ENDPOINT_LITERAL) {
			/* not implemented */
			LOG(1 - LOG_ANCHORING, "%s: not implemented\n", __func__);
			return 0;
		}

		const int res = f->u.literal.c <= '\n' && t->u.literal.c >= '\n';
		LOG(1 - LOG_ANCHORING, "%s: RANGE res %d\n", __func__, res);
		return res;
	}

	default:
		break;
	}

	return 0;
}

struct anchoring_env {
	enum re_flags re_flags;

	/* When sweeping forward, is the node being evaluated past
	 * a point where input must have been consumed? This is used
	 * to mark start anchors as unsatisfiable. */
	int past_always_consuming;

	/* Corresponding flag for end anchors while sweeping backward. */
	int followed_by_consuming;

	/* Special case for detecting '$[^a]', which matches "\n" with
	 * a capture group 0 of (0,1) in PCRE. */
	int followed_by_consuming_newline;

	/* Flag for tracking whether we're in a part of the subtree that
	 * is always before a start anchor. This influences satisfiability
	 * and edge cases like '()*^'. */
	int before_start_anchor;

	/* Flag used to detect and reject the awkward case in '$[^a]',
	 * where (according to PCRE) the character class after the '$'
	 * should match the literal newline, but nothing else, and only
	 * once. Because $ is actually a zero-width assertion that
	 * execution is either at the end of input or a trailing
	 * newline, it has the rather surprising result that '$[^a]'
	 * will not match "x" but *will* match "x\n" (because it has a $
	 * before a trailing newline, and because the newline matches
	 * the non-skippable [^a]). We just return an unsupported
	 * error for this case. */
	enum newline_after_end_anchor_state {
		NAEAS_NONE,
		NAEAS_WOULD_MATCH_ONCE,
	} newline_after_end_anchor_state;

	int after_end_anchor;
};

/* Tree walker that analyzes the AST, marks which nodes and subtrees are
 * anchored at the start and/or end, and determines which subtrees are
 * unsatisfiable due to start anchoring.
 *
 * The return value represents the status of the subtree, the flags on
 * the *env represent the state of the execution path so far, such as
 * whether input has always been consumed by the time the current node
 * is evaluated (which makes start anchors unmatchable).
 *
 * A second pass, analysis_iter_reverse_anchoring, checks for
 * unsatisfiability due to end anchors, */
static enum ast_analysis_res
analysis_iter_anchoring(struct anchoring_env *env, struct ast_expr *n)
{
	enum ast_analysis_res res;

	/*
	 * Analyze the AST for anchors that make subtrees or the entire
	 * AST unsatisfiable.
	 */

	LOG(3 - LOG_ANCHORING, "%s: node %p, type %s\n",
	    __func__, (void *)n, ast_node_type_name(n->type));

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
			set_flags(n, AST_FLAG_ANCHORED_START);

			if (env->past_always_consuming) {
				/*
				 * Start anchors that are not in a FIRST position
				 * almost perfectly match unsatisfiable start
				 * anchors, with the exception of multiple start
				 * anchors in a row (e.g. `^^ab$`). We can't just
				 * treat them as nullable either, because it messes
				 * up the results for linking.
				 *
				 * Instead, we check for start anchors occurring on
				 * a path where input has always been consumed already,
				 * via the env->past_always_consuming flag.
				 */
				LOG(3 - LOG_ANCHORING,
				    "%s: START anchor & past_any_consuming, setting UNSATISFIABLE\n",
				    __func__);
				set_flags(n, AST_FLAG_UNSATISFIABLE);
				return AST_ANALYSIS_UNSATISFIABLE;
			}
			break;

		case AST_ANCHOR_END:
			set_flags(n, AST_FLAG_ANCHORED_END);
			env->after_end_anchor = 1;
			LOG(3 - LOG_ANCHORING,
			    "%s: END anchor\n", __func__);
			if (n->u.anchor.is_end_nl && !(env->re_flags & RE_ANCHORED)) {
				set_flags(n, AST_FLAG_END_NL);
				if (env->newline_after_end_anchor_state == NAEAS_NONE) {
					env->newline_after_end_anchor_state = NAEAS_WOULD_MATCH_ONCE;
				}
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
		if (n->u.literal.c == '\n') {
			set_flags(n, AST_FLAG_MATCHES_1NEWLINE);

			if (env->newline_after_end_anchor_state == NAEAS_WOULD_MATCH_ONCE) {
				LOG(3 - LOG_ANCHORING,
				    "%s: LITERAL: rejecting non-optional newline match after $ as unsupported\n",
				    __func__);
				set_flags(n, AST_FLAG_UNSATISFIABLE);
				return AST_ANALYSIS_ERROR_UNSUPPORTED_PCRE;
			}
		}
		break;

	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
		if (can_consume_single_newline(n)) {
			set_flags(n, AST_FLAG_MATCHES_1NEWLINE);
		}
		break;		/* handled outside switch/case */

	case AST_EXPR_CONCAT: {
		size_t i;
		for (i = 0; i < n->u.concat.count; i++) {
			struct ast_expr *child = n->u.concat.n[i];

			/*
			 * If we were to encounter a tombstone here, the entire concat
			 * would be unsatisfiable because all children must be matched.
			 * However the AST rewriting resolved these already.
			 */
			assert(child->type != AST_EXPR_TOMBSTONE);

			/*
			 * Also removed from CONCAT during AST rewriting.
			 */
			assert(child->type != AST_EXPR_EMPTY);

			LOG(3 - LOG_CONCAT_FLAGS,
			    "%s: %p: %lu: %p -- past_always_consuming %d\n",
			    __func__, (void *)n, i, (void *)child,
			    env->past_always_consuming);

			struct anchoring_env child_env = *env;
			res = analysis_iter_anchoring(&child_env, child);

			if (res != AST_ANALYSIS_OK &&
			    res != AST_ANALYSIS_UNSATISFIABLE) { /* unsat is handled below */
				return res;
			}

			/* If a subtree is unsatisfiable but can be
			 * repeated zero times, just skip it. */
			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				if (is_nullable(child)) {
					assert(child->flags & AST_FLAG_UNSATISFIABLE);
					/* skip */
				} else if ((child->flags & AST_FLAG_ANCHORED_START) && env->past_always_consuming) {
					/* This could also be (n->flags & AST_FLAG_ANCHORED_START), except
					 * the anchoring flags flow upward in a later pass. */
					LOG(3 - LOG_ANCHORING,
					    "%s: unsatisfiable child %p past_always_consuming and start-anchored concat %p -> UNSATISFIABLE\n",
					    __func__, (void *)child, (void *)n);
					set_flags(n, AST_FLAG_UNSATISFIABLE);
				}
			}

			/* If we were previously not past any nodes that always
			 * consume input, but the child's analysis set that flag,
			 * then copy the setting if the child cannot be skipped. */
			if (!env->past_always_consuming && child_env.past_always_consuming
			    && !is_nullable(child)) {
				LOG(3 - LOG_ANCHORING,
				    "%s: setting past_always_consuming due to child %p's analysis\n",
				    __func__, (void *)child);
				env->past_always_consuming = 1;
			}

			env->newline_after_end_anchor_state = child_env.newline_after_end_anchor_state;
			env->after_end_anchor = child_env.after_end_anchor;
		}

		/* flow ANCHORED_START and ANCHORED_END flags upward */
		{
			int after_always_consumes = 0;

			for (i = 0; i < n->u.concat.count; i++) {
				struct ast_expr *child = n->u.concat.n[i];
				if (child->flags & AST_FLAG_ANCHORED_START) {
					LOG(3 - LOG_ANCHORING,
					    "%s: setting ANCHORED_START due to child %zu (%p)'s analysis\n",
					    __func__, i, (void *)child);
					set_flags(n, AST_FLAG_ANCHORED_START);

					if (after_always_consumes) {
						LOG(3 - LOG_ANCHORING,
						    "%s: setting %p and child %p UNSATISFIABLE due to ^-anchored child that always consumes input\n",
						    __func__, (void *)n, (void *)child);
						set_flags(n, AST_FLAG_UNSATISFIABLE);
						set_flags(child, AST_FLAG_UNSATISFIABLE);
					}
				}

				if (always_consumes_input(child)) {
					LOG(3 - LOG_ANCHORING,
					    "%s: child %zd always consumes input\n", __func__, i);
					after_always_consumes = 1;
				}
			}
		}
		{
			for (i = n->u.concat.count; i > 0; i--) {
				struct ast_expr *child = n->u.concat.n[i - 1];
				if (child->flags & AST_FLAG_ANCHORED_END) {
					LOG(3 - LOG_ANCHORING,
					    "%s: setting ANCHORED_END due to child %zu (%p)'s analysis\n",
					    __func__, i, (void *)child);
					set_flags(n, child->flags & END_ANCHOR_FLAG_MASK);
				}
			}
		}

		/* flow ANCHORED_START and ANCHORED_END flags laterally and downward */
		{
			/* If a node is ^ anchored, set the ^ flag on any nodes preceding
			 * it as long as none of them always consume input. */
			int anchored = 0;
			for (i = n->u.concat.count; i > 0; i--) {
				struct ast_expr *child = n->u.concat.n[i - 1];
				if (child->flags & AST_FLAG_ANCHORED_START) {
					anchored = 1;
				} else if (anchored) {
					set_flags_subtree(child, AST_FLAG_ANCHORED_START);
				}

				if (always_consumes_input(child)) {
					break;
				}
			}

			/* Same with $ anchors, but use END_ANCHOR_FLAG_MASK so that both
			 * the AST_FLAG_ANCHORED_END and AST_FLAG_END_NL flags are copied. */
			enum ast_flags end_anchor_flags = 0;
			for (i = 0; i < n->u.concat.count; i++) {
				struct ast_expr *child = n->u.concat.n[i];
				if (child->flags & END_ANCHOR_FLAG_MASK) {
					end_anchor_flags = child->flags & END_ANCHOR_FLAG_MASK;
				} else if (anchored) {
					set_flags_subtree(child, end_anchor_flags);
				}

				if (always_consumes_input(child)) {
					break;
				}
			}
		}

		/* Anything after an end anchor that always consumes input is unsatisfiable.
		 * Anything that can be skipped will be pruned later. */
		int after_end_anchor = 0;
		for (i = 0; i < n->u.concat.count; i++) {
			struct ast_expr *child = n->u.concat.n[i];
			if (after_end_anchor) {
				if (always_consumes_input(child)) {
					LOG(3 - LOG_ANCHORING,
					    "%s: after_end_anchor & ALWAYS_CONSUMES on child %p -> UNSATISFIABLE\n",
					    __func__, (void *)child);
					set_flags(child, AST_FLAG_UNSATISFIABLE);
				}

				if (child->type == AST_EXPR_REPEAT
				    && (child->flags & AST_FLAG_UNSATISFIABLE)
				    && child->u.repeat.min == 0) {
					prune_repeat_node(child);
				}
			} else if (!after_end_anchor
			    && child->flags & AST_FLAG_ANCHORED_END
			    && !is_nullable(child)) {
				if (!after_end_anchor) {
					LOG(3 - LOG_ANCHORING,
					    "%s: setting after_end_anchor due to child %p's analysis\n",
					    __func__, (void *)child);
					after_end_anchor = 1;
				}
			}
		}

		for (i = 0; i < n->u.concat.count; i++) {
			struct ast_expr *child = n->u.concat.n[i];
			if (can_consume_single_newline(child)) {
				set_flags(n, AST_FLAG_MATCHES_1NEWLINE);
			}
		}

		break;
	}

	case AST_EXPR_ALT: {
		int any_sat = 0;
		int all_set_past_always_consuming = 1;

		int all_start_anchored = 1;
		int all_end_anchored = 1;

		assert(n->u.alt.count > 0);

		/* If all children set ANCHORED_START, ANCHORED_END, or past_always_consuming,
		 * then propagate that upward. If any set CAN_CONSUME then set that.
		 * If no children are satisfiable, then set UNSATISFIABLE. */
		for (size_t i = 0; i < n->u.alt.count; i++) {
			struct ast_expr *child = n->u.alt.n[i];

			/* Tombstones should have been removed during ast_rewrite, though
			 * (unlike in AST_EXPR_CONCAT) empty nodes can appear in an ALT,
			 * because of expressions like `x|`. */
			assert(child->type != AST_EXPR_TOMBSTONE);

			struct anchoring_env child_env = *env;

			res = analysis_iter_anchoring(&child_env, child);

			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				/* will be ignored later */
				LOG(3 - LOG_ANCHORING,
				    "%s: ALT child %zu (%p) returned UNSATISFIABLE, will be ignored later\n",
				    __func__, i, (void *)n->u.alt.n[i]);
				assert(child->flags & AST_FLAG_UNSATISFIABLE);
			} else if (res == AST_ANALYSIS_OK) {
				if (child->type == AST_EXPR_REPEAT
				    && /*child->u.repeat.min == 0 &&*/ child->u.repeat.max == 0) {
					LOG(3 - LOG_ANCHORING,
					    "%s: ignoring pruned REPEAT node\n", __func__);
				} else {
					all_set_past_always_consuming &= child_env.past_always_consuming;
					any_sat = 1;
				}
			} else if (res == AST_ANALYSIS_ERROR_UNSUPPORTED_CAPTURE
			    || res == AST_ANALYSIS_ERROR_UNSUPPORTED_PCRE) {
				continue;
			} else {
				return res;
			}

			if (env->after_end_anchor
			    && always_consumes_input(child)) {
				LOG(3 - LOG_ANCHORING,
				    "%s: ALT child %zd is after_end_anchor and always_consumes_input => UNSATISFIABLE\n",
				    __func__, i);
				set_flags(child, AST_FLAG_UNSATISFIABLE);
			}

			if (!(child->flags & AST_FLAG_UNSATISFIABLE)) { /* ignore unsat nodes */
				if (!(child->flags & AST_FLAG_ANCHORED_START)) {
					all_start_anchored = 0;
				}
				if (!(child->flags & AST_FLAG_ANCHORED_END)) {
					all_end_anchored = 0;
				}
			}

			if (child->flags & AST_FLAG_MATCHES_1NEWLINE) {
				set_flags(n, AST_FLAG_MATCHES_1NEWLINE);
			}
		}

		if (!env->past_always_consuming && all_set_past_always_consuming) {
			LOG(3 - LOG_ANCHORING,
			    "%s: ALT: all_set_past_always_consuming -> setting env->past_always_consuming\n",
			    __func__);
			env->past_always_consuming = 1;
		}

		/* An ALT group is only unsatisfiable if they ALL are. */
		if (!any_sat) {
			LOG(3 - LOG_ANCHORING,
			    "%s: ALT: !any_sat -> UNSATISFIABLE\n", __func__);
			set_flags(n, AST_FLAG_UNSATISFIABLE);
			return AST_ANALYSIS_UNSATISFIABLE;
		}

		if (all_start_anchored) {
			LOG(3 - LOG_ANCHORING,
			    "%s: ALT: all_start_anchored -> ANCHORED_START\n", __func__);
			set_flags(n, AST_FLAG_ANCHORED_START);
		}
		if (all_end_anchored) {
			LOG(3 - LOG_ANCHORING,
			    "%s: ALT: all_end_anchored -> ANCHORED_END\n", __func__);
			/* FIXME: AST_FLAG_END_NL: need to determine how this interacts
			 * with alt nodes. `^(?:(a)\z|(a)$)` */
			set_flags(n, AST_FLAG_ANCHORED_END);
		}

		break;
	}

	case AST_EXPR_REPEAT:;
		const int orig_after_end_anchor = env->after_end_anchor;

		if (n->u.repeat.min == 0) {
			/* Call the child with a copy of the anchoring env,
			 * since any subtree repeated zero or more times can
			 * always be ignored. */
			struct anchoring_env child_env = *env;
			res = analysis_iter_anchoring(&child_env, n->u.repeat.e);
		} else {
			res = analysis_iter_anchoring(env, n->u.repeat.e);
		}

		/*
		 * This logic corresponds to the equivalent case for tombstone nodes
		 * during the (earlier) AST rewrite pass, except here we deal with
		 * unsatisfiable nodes rather than just tombstones.
		 */
		if (res == AST_ANALYSIS_UNSATISFIABLE) {
			/* If the subtree is unsatisfiable but has a repetition
			 * min of 0, then just set the UNSATISFIABLE flag and
			 * continue -- it shouldn't cause the AST as a whole to
			 * get rejected, but that subtree will be skipped later.
			 *
			 * An example of this case is /^a($b)*c$/: while the
			 * ($b)* can never match anything, the AST can still
			 * have a valid result when repeated zero times. */
			if (n->u.repeat.min == 0) {
				LOG(3 - LOG_ANCHORING,
				    "%s: REPEAT: UNSATISFIABLE but can be repeated 0 times, ignoring\n", __func__);
				prune_repeat_node(n);
				break;
			} else if (n->u.repeat.min > 0) {
				set_flags(n, AST_FLAG_UNSATISFIABLE);
				LOG(3 - LOG_ANCHORING,
				    "%s: REPEAT: UNSATISFIABLE and must be repeated >0 times -> UNSATISFIABLE\n", __func__);
				return AST_ANALYSIS_UNSATISFIABLE;
			}
		} else if (res != AST_ANALYSIS_OK) {
			if (n->u.repeat.min == 0) {
				LOG(3 - LOG_ANCHORING,
				    "%s: REPEAT: analysis on child returned %d, setting repeat count to 0\n", __func__, res);
				prune_repeat_node(n);
			} else {
				LOG(3 - LOG_ANCHORING,
				    "%s: REPEAT: analysis on child returned %d\n", __func__, res);
				return res;
			}
		}

		if (can_consume_single_newline(n->u.repeat.e)) {
			set_flags(n, AST_FLAG_MATCHES_1NEWLINE);
		}

		if (n->u.repeat.e->flags & AST_FLAG_ANCHORED_START && n->u.repeat.min > 0) {
			LOG(3 - LOG_ANCHORING,
			    "%s: REPEAT: repeating ANCHORED_START subtree >0 times -> ANCHORED_START\n", __func__);
			set_flags(n, AST_FLAG_ANCHORED_START);
			n->u.repeat.max = 1;
		}

		if (can_consume_single_newline(n->u.repeat.e)) {
			set_flags(n, AST_FLAG_MATCHES_1NEWLINE);
		}

		if (n->u.repeat.e->flags & AST_FLAG_ANCHORED_END && n->u.repeat.min > 0) {
			LOG(3 - LOG_ANCHORING,
			    "%s: REPEAT: repeating ANCHORED_END subtree >0 times -> ANCHORED_END\n", __func__);
			set_flags(n, n->u.repeat.e->flags & END_ANCHOR_FLAG_MASK);
			n->u.repeat.max = 1;
		}

		/* If there's a {0,_} repeat group after an end anchor that always consumes
		 * input, set its max count to 0. This should be based on the after_end_anchor
		 * flag value *before* analyzing the repeated subtree. */
		if (orig_after_end_anchor
		    && always_consumes_input(n)
		    && n->u.repeat.min == 0) {
			prune_repeat_node(n);
 		}

		break;

	case AST_EXPR_GROUP:
		res = analysis_iter_anchoring(env, n->u.group.e);

		/* This flows anchoring flags upward even when the node
		 * is unsatisfiable, because that info can impact how
		 * the node's unsatisfiability is handled. */
#define PROPAGATE_CHILD_FLAGS(TAG, N, CHILD)				\
		do {							\
			struct ast_expr *child = CHILD;			\
			if (child->flags & AST_FLAG_ANCHORED_START) {   \
				set_flags(N, AST_FLAG_ANCHORED_START);	\
			}						\
			if (child->flags & AST_FLAG_ANCHORED_END) {	\
				set_flags(N, AST_FLAG_ANCHORED_END);	\
			}						\
			if (res == AST_ANALYSIS_UNSATISFIABLE) {	\
				LOG(3 - LOG_ANCHORING,			\
				    "%s: %s: setting UNSATISFIABLE due to unsatisfiable child\n", \
				    __func__, TAG);			\
				set_flags(N, AST_FLAG_UNSATISFIABLE);	\
			}						\
			if (res != AST_ANALYSIS_OK) {			\
				return res;				\
			}						\
		} while(0)

		PROPAGATE_CHILD_FLAGS("GROUP", n, n->u.group.e);

		if (n->u.group.e->flags & AST_FLAG_MATCHES_1NEWLINE) {
			set_flags(n, AST_FLAG_MATCHES_1NEWLINE);
		}

		break;

	case AST_EXPR_SUBTRACT:
		res = analysis_iter_anchoring(env, n->u.subtract.a);
		if (n->u.subtract.a->flags & AST_FLAG_ANCHORED_START) {
			set_flags(n, AST_FLAG_ANCHORED_START);
		}
		if (n->u.subtract.a->flags & AST_FLAG_ANCHORED_END) {
			set_flags(n, n->u.subtract.a->flags & END_ANCHOR_FLAG_MASK);
		}

		if (env->newline_after_end_anchor_state == NAEAS_WOULD_MATCH_ONCE) {
			LOG(3 - LOG_ANCHORING,
			    "%s: SUBTRACT: rejecting non-optional newline match after $ as unsupported\n",
			    __func__);
			set_flags(n, AST_FLAG_UNSATISFIABLE);
			return AST_ANALYSIS_ERROR_UNSUPPORTED_PCRE;
		}

		if (res != AST_ANALYSIS_OK) {
			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				set_flags(n, AST_FLAG_UNSATISFIABLE);
			}
			return res;
		}

		res = analysis_iter_anchoring(env, n->u.subtract.b);
		if (res != AST_ANALYSIS_OK) {
			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				LOG(3 - LOG_ANCHORING,
				    "%s: SUBTRACT: setting UNSATISFIABLE due to unsatisfiable child\n",
				    __func__);
				set_flags(n, AST_FLAG_UNSATISFIABLE);
			}
			return res;
		}
		if (can_consume_single_newline(n->u.repeat.e)) {
			set_flags(n, AST_FLAG_MATCHES_1NEWLINE);
		}

		break;

	default:
		assert(!"unreached");
	}

	if (always_consumes_input(n)) {
		LOG(3 - LOG_ANCHORING,
		    "%s: always_consumes_input(%p) -> setting past_any_consuming\n",
		    __func__, (void *)n);
		env->past_always_consuming = 1;
	}

	if (n->flags & AST_FLAG_UNSATISFIABLE) {
		return AST_ANALYSIS_UNSATISFIABLE;
	}

	return AST_ANALYSIS_OK;
}

static enum ast_analysis_res
analysis_iter_reverse_anchoring(struct anchoring_env *env, struct ast_expr *n)
{
	enum ast_analysis_res res;

	/*
	 * Second pass, in reverse, specifically checking for end-anchored
	 * subtrees that are unsatisfiable because they are followed by
	 * nodes that always consume input.
	 *
	 * Also check for subtrees that always consume input appearing
	 * before a start anchor and mark them as unsatisfiable.
	 */

	LOG(3 - LOG_ANCHORING, "%s: node %p, type %s, followed_by_consuming %d, before_start_anchor %d\n",
	    __func__, (void *)n, ast_node_type_name(n->type),
	    env->followed_by_consuming, env->before_start_anchor);

	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_TOMBSTONE:
		break;

	case AST_EXPR_ANCHOR:
		switch (n->u.anchor.type) {
		case AST_ANCHOR_START:
			LOG(3 - LOG_ANCHORING,
			    "%s: START anchor, setting before_start_anchor = 1\n",
			    __func__);
			env->before_start_anchor = 1;
			break;

		case AST_ANCHOR_END:
			/* This should already be set during forward pass, but
			 * may have been skipped when evaluation moving forward
			 * rejected this subtree as unsatisfiable (but evaluating
			 * moving in reverse hasn't). */
			set_flags(n, AST_FLAG_ANCHORED_END);

			if (env->followed_by_consuming) {
				if (env->followed_by_consuming_newline) {
					LOG(3 - LOG_ANCHORING,
					    "%s: END anchor & followed_by_consuming, returning UNSUPPORTED_PCRE\n",
					    __func__);
					return AST_ANALYSIS_ERROR_UNSUPPORTED_PCRE;
				} else {
					LOG(3 - LOG_ANCHORING,
					    "%s: END anchor & followed_by_consuming, setting UNSATISFIABLE\n",
					    __func__);
					set_flags(n, AST_FLAG_UNSATISFIABLE);
					return AST_ANALYSIS_UNSATISFIABLE;
				}
			}

			break;

		default:
			assert(!"unreached");
		}
		break;

	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
		if (env->before_start_anchor) {
			LOG(3 - LOG_ANCHORING,
			    "%s: single node consuming input before_start_anchor => UNSATISFIABLE\n",
			    __func__);
			set_flags(n, AST_FLAG_UNSATISFIABLE);
			return AST_ANALYSIS_UNSATISFIABLE;
		}
		break;		/* handled outside switch/case */

	case AST_EXPR_CONCAT: {
		size_t i;
		int start_anchored = 0;
		for (i = n->u.concat.count; i > 0; i--) {
			struct ast_expr *child = n->u.concat.n[i - 1];
			assert(child->type != AST_EXPR_TOMBSTONE);

			LOG(3 - LOG_CONCAT_FLAGS,
			    "%s: %p: %lu: %p -- followed_by_consuming %d\n",
			    __func__, (void *)n, i, (void *)child,
			    env->followed_by_consuming);

			struct anchoring_env child_env = *env;
			res = analysis_iter_reverse_anchoring(&child_env, child);

			/* If a subtree is unsatisfiable but can be
			 * repeated zero times, just skip it. */
			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				LOG(3 - LOG_CONCAT_FLAGS,
				    "%s: %p: child %lu (%p) returned UNSATISFIABLE\n",
				    __func__, (void *)n, i, (void *)child);
				if (is_nullable(child)) { /* can be skipped */
					assert(child->flags & AST_FLAG_UNSATISFIABLE);
				} else if (can_consume_input(child)) {
					LOG(3 - LOG_ANCHORING,
					    "%s: unsatisfiable, non-nullable child that can consume input %p -> setting %p to UNSATISFIABLE\n",
					    __func__, (void *)child, (void *)n);
					set_flags(n, AST_FLAG_UNSATISFIABLE);
				} else if (always_consumes_input(n) && env->followed_by_consuming) {
					LOG(3 - LOG_ANCHORING,
					    "%s: unsatisfiable, non-nullable child %p in concat that always consumes input %p -> UNSATISFIABLE\n",
					    __func__, (void *)child, (void *)n);
					set_flags(n, AST_FLAG_UNSATISFIABLE);
				} else if (n->flags & AST_FLAG_ANCHORED_END && env->followed_by_consuming) {
					LOG(3 - LOG_ANCHORING,
					    "%s: unsatisfiable, end-anchored child %p in concat %p -> UNSATISFIABLE\n",
					    __func__, (void *)child, (void *)n);
					set_flags(n, AST_FLAG_UNSATISFIABLE);
				}
			} else if (res != AST_ANALYSIS_OK) {
				LOG(3 - LOG_ANCHORING,
				    "%s: CONCAT: got res of %d, bubbling up\n", __func__, res);
				return res;
			}

			if (child->flags & AST_FLAG_ANCHORED_START) {
				LOG(3 - LOG_ANCHORING,
				    "%s: CONCAT: child %zd is ANCHORED_START\n", __func__, i);
				start_anchored = 1;
			}

			/* If we were previously not past any nodes that always
			 * consume input, but the child's analysis set that flag,
			 * then copy the setting if the child cannot be skipped. */

			if (!env->followed_by_consuming && child_env.followed_by_consuming
			    && !is_nullable(child)) {
				LOG(3 - LOG_ANCHORING,
				    "%s: setting followed_by_consuming due to child %p's analysis\n",
				    __func__, (void *)child);
				env->followed_by_consuming = 1;
			}

			if (!env->followed_by_consuming_newline &&
			    (child_env.followed_by_consuming_newline
				|| child->flags & AST_FLAG_MATCHES_1NEWLINE)) {
				LOG(3 - LOG_ANCHORING,
				    "%s: setting followed_by_consuming_newline due to child %p's analysis\n",
				    __func__, (void *)child);
				env->followed_by_consuming_newline = 1;
			}

			if (start_anchored) {
				LOG(3 - LOG_ANCHORING,
				    "%s: earliest unskipped child had ANCHORED_START, bubbling up\n",
				    __func__);
				set_flags(n, AST_FLAG_ANCHORED_START);
			}

			if (!env->before_start_anchor && child_env.before_start_anchor
			    && !is_nullable(child)) {
				LOG(3 - LOG_ANCHORING,
				    "%s: setting before_start_anchor due to child %p's analysis\n",
				    __func__, (void *)child);
				env->before_start_anchor = 1;
			}
		}

		break;
	}

	case AST_EXPR_ALT: {
		int any_sat = 0;
		int all_set_followed_by_consuming = 1;
		int any_set_followed_by_consuming_newline = 0;
		int all_set_before_start_anchor = 1;

		assert(n->u.alt.count > 0);

		/* If all children set ANCHORED_START, ANCHORED_END, or past_always_consuming,
		 * then propagate that upward. If any set CAN_CONSUME then set that.
		 * If no children are satisfiable, then set UNSATISFIABLE. */
		for (size_t i = 0; i < n->u.alt.count; i++) {
			/* Same assertions as in the AST_EXPR_CONCAT case -- these should
			 * have been eliminated by the end of ast_rewrite. */
			struct ast_expr *child = n->u.alt.n[i];
			assert(child->type != AST_EXPR_TOMBSTONE);

			struct anchoring_env child_env = *env;
			res = analysis_iter_reverse_anchoring(&child_env, child);

			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				/* will be ignored later */
				LOG(3 - LOG_ANCHORING,
				    "%s: ALT child %zu (%p) returned UNSATISFIABLE, will be ignored later\n",
				    __func__, i, (void *)n->u.alt.n[i]);
				assert(child->flags & AST_FLAG_UNSATISFIABLE);
			} else if (res == AST_ANALYSIS_OK) {
				if (0 && child->type == AST_EXPR_REPEAT
				    && child->u.repeat.max == 0) {
					LOG(3 - LOG_ANCHORING,
					    "%s: ignoring pruned REPEAT node\n", __func__);
				} else {
					all_set_followed_by_consuming &= child_env.followed_by_consuming;
					all_set_before_start_anchor &= child_env.before_start_anchor;
					any_set_followed_by_consuming_newline |= child_env.followed_by_consuming_newline;
					any_sat = 1;
				}
			} else if (res == AST_ANALYSIS_ERROR_UNSUPPORTED_CAPTURE
			    || res == AST_ANALYSIS_ERROR_UNSUPPORTED_PCRE) {
				LOG(3 - LOG_ANCHORING, "%s: got res of UNSUPPORTED_*, bubbling up\n", __func__);
				return res;
			} else {
				return res;
			}
		}

		if (!env->followed_by_consuming && all_set_followed_by_consuming) {
			LOG(3 - LOG_ANCHORING,
			    "%s: ALT: all_set_followed_by_consuming -> setting env->followed_by_consuming\n",
			    __func__);
			env->followed_by_consuming = 1;
		}

		if (!env->followed_by_consuming_newline && any_set_followed_by_consuming_newline) {
			LOG(3 - LOG_ANCHORING,
			    "%s: ALT: any_set_followed_by_consuming_newline -> setting env->followed_by_consuming_newline for feature PCRE rejection\n",
			    __func__);
			env->followed_by_consuming_newline = 1;
		}

		if (!env->before_start_anchor && all_set_before_start_anchor) {
			LOG(3 - LOG_ANCHORING,
			    "%s: ALT: all_set_before_start_anchor -> setting env->before_start_anchor and ANCHORED_START\n",
			    __func__);
			env->before_start_anchor = 1;
			set_flags(n, AST_FLAG_ANCHORED_START);
		}

		/* An ALT group is only unsatisfiable if they ALL are. */
		if (!any_sat) {
			LOG(3 - LOG_ANCHORING,
			    "%s: ALT: !any_sat -> UNSATISFIABLE\n", __func__);
			set_flags(n, AST_FLAG_UNSATISFIABLE);
			return AST_ANALYSIS_UNSATISFIABLE;
		}

		break;
	}

	case AST_EXPR_REPEAT:
		if (n->u.repeat.min == 0) {
			/* Call the child with a copy of the anchoring env,
			 * since any subtree repeated zero or more times can
			 * always be ignored. */
			struct anchoring_env child_env = *env;
			res = analysis_iter_reverse_anchoring(&child_env, n->u.repeat.e);
		} else {
			res = analysis_iter_reverse_anchoring(env, n->u.repeat.e);
		}

		if (res == AST_ANALYSIS_UNSATISFIABLE) {
			if (n->u.repeat.min == 0) {
				LOG(3 - LOG_ANCHORING,
				    "%s: REPEAT: UNSATISFIABLE but can be repeated 0 times, ignoring\n", __func__);
				prune_repeat_node(n);
				break;
			} else if (n->u.repeat.min > 0) {
				LOG(3 - LOG_ANCHORING,
				    "%s: REPEAT: UNSATISFIABLE and must be repeated >0 times -> UNSATISFIABLE\n", __func__);
				set_flags(n, AST_FLAG_UNSATISFIABLE);
				return AST_ANALYSIS_UNSATISFIABLE;
			}
		} else if (res != AST_ANALYSIS_OK) {
			LOG(3 - LOG_ANCHORING,
			    "%s: REPEAT: analysis on child returned %d\n", __func__, res);
			return res;
		}

		/* If there's a repeat group that always matches anything before a start anchor,
		 * set its max count to 0. */
		if (env->before_start_anchor
		    && is_nullable(n)
		    && always_consumes_input(n)) {
			LOG(3 - LOG_ANCHORING,
			    "%s: REPEAT: repeated group that consumes input before ^, setting max count to 0\n",
			    __func__);
			prune_repeat_node(n);
		}

		break;

	case AST_EXPR_GROUP:
		res = analysis_iter_reverse_anchoring(env, n->u.group.e);
		if (res != AST_ANALYSIS_OK) {
			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				LOG(3 - LOG_ANCHORING,
				    "%s: setting UNSATISFIABLE on %p due to child %p\n",
				    __func__, (void *)n, (void *)n->u.group.e);
				set_flags(n, AST_FLAG_UNSATISFIABLE);
			}
			return res;
		}

		if (n->u.group.e->flags & ANCHOR_FLAG_MASK) {
			LOG(3 - LOG_ANCHORING,
			    "%s: bubbling up anchoring flags from %p to %p\n",
			    __func__, (void *)n->u.group.e, (void *)n);
			set_flags(n, n->u.group.e->flags & ANCHOR_FLAG_MASK);
		}

		break;

	case AST_EXPR_SUBTRACT:
		res = analysis_iter_reverse_anchoring(env, n->u.subtract.a);
		if (res != AST_ANALYSIS_OK) {
			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				LOG(3 - LOG_ANCHORING,
				    "%s: setting UNSATISFIABLE on %p due to child %p\n",
				    __func__, (void *)n, (void *)n->u.subtract.a);
				set_flags(n, AST_FLAG_UNSATISFIABLE);
			}
			return res;
		}

		res = analysis_iter_reverse_anchoring(env, n->u.subtract.b);
		if (res != AST_ANALYSIS_OK) {
			if (res == AST_ANALYSIS_UNSATISFIABLE) {
				set_flags(n, AST_FLAG_UNSATISFIABLE);
			}
			return res;
		}
		break;

	default:
		assert(!"unreached");
	}

	if (always_consumes_input(n) && !env->followed_by_consuming) {
		LOG(3 - LOG_ANCHORING,
		    "%s: always_consumes_input(%p) -> setting followed_by_consuming\n",
		    __func__, (void *)n);
		env->followed_by_consuming = 1;
	}

	if (n->flags & AST_FLAG_UNSATISFIABLE) {
		return AST_ANALYSIS_UNSATISFIABLE;
	}

	if (n->flags & AST_FLAG_CAN_CONSUME && matches_newline(n)) {
		env->followed_by_consuming_newline = 1;
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

			if (can_consume_input(child) || (child->flags & AST_FLAG_ANCHORED_START)) {
				if (!is_nullable(child)) { break; }
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
		 *
		 * It probably makes sense to not go further
		 * here because the top layer of the repeated section
		 * should only link to the global start once. */

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

		for (i = n->u.concat.count; i > 0; i--) {
			struct ast_expr *child = n->u.concat.n[i - 1];
			assign_lasts(child);
			if (can_consume_input(child) || (child->flags & AST_FLAG_ANCHORED_END)) {
				if (!is_nullable(child)) { break; }
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
		 *
		 * It probably makes sense to not go further
		 * here because the top layer of the repeated section
		 * should only link to the global start once. */

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

static int
is_only_anchors(struct ast_expr *expr)
{
	if (can_consume_input(expr)) { return 0; }

	switch (expr->type) {
	case AST_EXPR_ANCHOR:
		return 1;

	case AST_EXPR_CONCAT:
		if (expr->u.concat.count == 0) { return 0; }
		for (size_t i = 0; i < expr->u.concat.count; i++) {
			if (!is_only_anchors(expr->u.concat.n[i])
			    && can_consume_input(expr->u.concat.n[i])) {
				return 0;
			}
		}
		return 1;

	case AST_EXPR_ALT:
		assert(expr->u.alt.count > 0);
		for (size_t i = 0; i < expr->u.alt.count; i++) {
			/* earlier matches will shadow later ones */
			if (is_only_anchors(expr->u.alt.n[i])) {
				return 1;
			}
		}
		return 0;

	case AST_EXPR_REPEAT:
		if (expr->u.repeat.min == 0 && expr->u.repeat.max == 0) {
			return 0;
		}
		return is_only_anchors(expr->u.repeat.e);

	case AST_EXPR_GROUP:
		return is_only_anchors(expr->u.group.e);

	default:
		break;
	}

	return 0;
}

/* Check for certain special cases with repetition that need to be
 * handled carefully or rejected as unsupported. */
static enum ast_analysis_res
analysis_iter_repetition(struct ast_expr *n, struct ast_expr *outermost_repeat_parent,
	int shadowed_by_previous_alt_case, struct ast_expr *repeat_plus_ancestor)
{
	enum ast_analysis_res res = AST_ANALYSIS_OK;

	LOG(3 - LOG_REPETITION_CASES, "%s: node %p, type %s, shadowed_by_previous_alt_case %d\n",
	    __func__, (void *)n, ast_node_type_name(n->type), shadowed_by_previous_alt_case);

	if (shadowed_by_previous_alt_case) {
		assert(outermost_repeat_parent == NULL
		    || outermost_repeat_parent->type == AST_EXPR_ALT);
	}

	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_TOMBSTONE:
	case AST_EXPR_ANCHOR:
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
		break;

	case AST_EXPR_CONCAT: {
		/* If this CONCAT array of nodes always consumes input, then
		 * it cannot be repeated empty, so it cannot produce the
		 * special case that needs outermost_repeat_parent for
		 * AST_EXPR_REPEAT's case below.
		 *
		 * An example input that needs this is 'x(()x)*' for "xx",
		 * because the 'x' prevents the outermost group
		 * from repeating and matching empty again after consuming
		 * a run of "x"s. */
		if (always_consumes_input(n)) {
			outermost_repeat_parent = NULL;
		}

		for (size_t i = 0; i < n->u.concat.count; i++) {
			res = analysis_iter_repetition(n->u.concat.n[i], outermost_repeat_parent,
			    shadowed_by_previous_alt_case, repeat_plus_ancestor);
			if (res != AST_ANALYSIS_OK) { return res; }
		}
		break;
	}

	case AST_EXPR_ALT: {
		/* See AST_EXPR_GROUP below for why this matters. */
		int new_shadowed_by_previous_alt_case = shadowed_by_previous_alt_case;

		LOG(3 - LOG_REPETITION_CASES,
		    "%s: ALT node %p, repeat_plus_ancestor %p\n",
		    __func__, (void *)n, (void *)repeat_plus_ancestor);

		for (size_t i = 0; i < n->u.alt.count; i++) {
			/* If this is an ALT inside of a repeated subtree that contains
			 * a capture, this will need special handling. */
			if (outermost_repeat_parent != NULL) {
				LOG(3 - LOG_REPETITION_CASES,
				    "%s: setting outermost_repeat_parent to %p for alt branch %zu, repeat_plus_ancestor %p\n",
				    __func__, (void *)n, i, (void *)repeat_plus_ancestor);
				outermost_repeat_parent = n;
			}

			if (is_nullable(n->u.alt.n[i]) || is_only_anchors(n->u.alt.n[i])) {
				LOG(3 - LOG_REPETITION_CASES,
				    "%s: setting new_shadowed_by_previous_alt_case for alt branch %zu, repeat_plus_ancestor %p\n",
				    __func__, i, (void *)repeat_plus_ancestor);
				new_shadowed_by_previous_alt_case = 1;
				if (repeat_plus_ancestor != NULL) {
					n->u.alt.nullable_alt_inside_plus_repeat = 1;
					assert(repeat_plus_ancestor->type == AST_EXPR_REPEAT);
					assert(repeat_plus_ancestor->u.repeat.min == 1);
					assert(repeat_plus_ancestor->u.repeat.max == AST_COUNT_UNBOUNDED);

					/* Repetition of an alt subtree which has a capture group child that
					 * only contains only* anchors is not handled properly yet. This
					 * isn't actually _useful_, it's just something that comes up
					 * in fuzzing, so reject it as an unsupported PCRE construct.
					 *
					 * An example input that triggers this is '^(($)|)+$' . */
					set_flags(n, AST_FLAG_UNSATISFIABLE);
					return AST_ANALYSIS_ERROR_UNSUPPORTED_PCRE;
				}
			}

			res = analysis_iter_repetition(n->u.alt.n[i],
			    outermost_repeat_parent,
			    new_shadowed_by_previous_alt_case,
			    repeat_plus_ancestor);
			if (res != AST_ANALYSIS_OK) { return res; }
		}
		break;
	}

	case AST_EXPR_REPEAT:
	{
		struct ast_expr *child = n->u.repeat.e;

		LOG(3 - LOG_REPETITION_CASES, "%s: REPEAT node %p, min %u max %u nullable? %d, !cci %d\n",
		    __func__, (void *)n, n->u.repeat.min, n->u.repeat.max,
		    is_nullable(child), can_consume_input(child));

		if (n->u.repeat.min == 1 &&
		    n->u.repeat.max == AST_COUNT_UNBOUNDED) {
			LOG(3 - LOG_REPETITION_CASES, "%s: setting repeat_plus_ancestor to %p\n",
			    __func__, (void *)n);
			repeat_plus_ancestor = n;
		} else {
			repeat_plus_ancestor = NULL;
		}

		/* Special cases for a repeated group that contains possibly empty captures,
		 * in order to correctly reflect their repeating one more time and capture
		 * at the end (but without an infinite loop).
		 *
		 * For example, '^((x?))*$' will always end up with capture groups 1 and 2
		 * at the end of the input for any number of "x"s, since the outermost ()*
		 * can always repeat once more time, consuming nothing, and clobber the
		 * existing captures. We mark repeated groups so that the compiled capture
		 * program can move saving the captures after the repetition, instead
		 * behaving like `^((?:x?)*(())$`.
		 *
		 * However, if the repeated subtree always consumes input, such as with
		 * '^(()a)+b$', then clear any passed in outermost_repeat_parent, because
		 * having to consume input will prevent that extra repetition of the
		 * empty captures. */
		if (always_consumes_input(n)) {
			res = analysis_iter_repetition(child, NULL, shadowed_by_previous_alt_case,
			    repeat_plus_ancestor);
		} else if (outermost_repeat_parent == NULL && n->u.repeat.max > 1) {
		        LOG(3 - LOG_REPETITION_CASES, "%s: recursing with outermost_repeat_parent set to %p\n",
			    __func__, (void *)n);
			res = analysis_iter_repetition(child, n, 0,
			    repeat_plus_ancestor);
		} else {
			LOG(3 - LOG_REPETITION_CASES, "%s: recursing with outermost_repeat_parent %p\n",
			    __func__, (void *)outermost_repeat_parent);
			res = analysis_iter_repetition(child, outermost_repeat_parent, shadowed_by_previous_alt_case,
			    repeat_plus_ancestor);
		}
		if (res != AST_ANALYSIS_OK) { return res; }
		break;
	}

	case AST_EXPR_GROUP:
		LOG(3 - LOG_REPETITION_CASES,
		    "%s: GROUP %p, repeat_plus_ancestor %p\n",
		    __func__, (void *)n, (void *)repeat_plus_ancestor);


		if (outermost_repeat_parent != NULL && (is_nullable(n) || !can_consume_input(n))) {
			int should_mark_repeated = 1;
			/* If the outermost_repeat_parent is an ALT node and a previous ALT subtree
			 * matching the empty string is shadowing this group, then do not mark it
			 * as repeated, because that can lead to incorrect handling in somewhat
			 * contrived regexes like '^(?:|(|x))*$'. */
			if (outermost_repeat_parent->type == AST_EXPR_ALT && shadowed_by_previous_alt_case) {
				LOG(3 - LOG_REPETITION_CASES,
				    "%s: hit group shadowed_by_previous_alt_case, skipping\n", __func__);
				should_mark_repeated = 0;
			}

			if (n->flags & (AST_FLAG_ANCHORED_START | AST_FLAG_ANCHORED_END)) {
				LOG(3 - LOG_REPETITION_CASES,
				    "%s: hit repeating anchor, skipping\n", __func__);
				should_mark_repeated = 0;
			}

			if (should_mark_repeated) {
				LOG(3 - LOG_REPETITION_CASES, "%s: setting group %u to repeated\n",
				    __func__, n->u.group.id);
				n->u.group.repeated = 1;
				assert(outermost_repeat_parent->type == AST_EXPR_REPEAT ||
				    outermost_repeat_parent->type == AST_EXPR_ALT);
				LOG(3 - LOG_REPETITION_CASES, "%s: setting contains_empty_groups on outermost_repeat_parent %p\n",
				    __func__, (void *)outermost_repeat_parent);
				if (outermost_repeat_parent->type == AST_EXPR_REPEAT) {
					outermost_repeat_parent->u.repeat.contains_empty_groups = 1;
				} else if (outermost_repeat_parent->type == AST_EXPR_ALT) {
					outermost_repeat_parent->u.alt.contains_empty_groups = 1;
				} else {
					assert(!"type mismatch");
				}
			}
		}

		if (repeat_plus_ancestor != NULL && (is_nullable(n) || !can_consume_input(n))) {
			assert(repeat_plus_ancestor->type == AST_EXPR_REPEAT
			    && repeat_plus_ancestor->u.repeat.min == 1
			    && repeat_plus_ancestor->u.repeat.max == AST_COUNT_UNBOUNDED);
			LOG(3 - LOG_REPETITION_CASES,
			    "%s: not yet implemented, skipping\n", __func__);
			/* return AST_ANALYSIS_ERROR_UNSUPPORTED_PCRE; */
		}

		res = analysis_iter_repetition(n->u.group.e, outermost_repeat_parent,
		    shadowed_by_previous_alt_case, repeat_plus_ancestor);
		if (res != AST_ANALYSIS_OK) { return res; }
		break;

	case AST_EXPR_SUBTRACT:
		res = analysis_iter_repetition(n->u.subtract.a, outermost_repeat_parent, shadowed_by_previous_alt_case,
			repeat_plus_ancestor);
		if (res != AST_ANALYSIS_OK) { return res; }
		res = analysis_iter_repetition(n->u.subtract.b, outermost_repeat_parent, shadowed_by_previous_alt_case,
			repeat_plus_ancestor);
		break;

	default:
		assert(!"unreached");
	}
	return res;
}

struct pincer_anchors_env {
	int always_consumes_or_anchored_start;
	int always_consumes_or_anchored_end;

	int alloc_fail;
	size_t used;
	size_t ceil;
	struct ast_expr **nodes;
};

enum analysis_direction { AD_FORWARD, AD_BACKWARD };

static int
subtree_contains_anchor(const struct ast_expr *expr, enum ast_anchor_type type)
{
	switch (expr->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
	case AST_EXPR_TOMBSTONE:
		return 0;

	case AST_EXPR_ANCHOR:
		return expr->u.anchor.type == type;

	case AST_EXPR_CONCAT:
		for (size_t i = 0; i < expr->u.concat.count; i++) {
			if (subtree_contains_anchor(expr->u.concat.n[i], type)) {
				return 1;
			}
		}
		return 0;
	case AST_EXPR_ALT:
		for (size_t i = 0; i < expr->u.alt.count; i++) {
			if (subtree_contains_anchor(expr->u.alt.n[i], type)) {
				return 1;
			}
		}
		return 0;

	case AST_EXPR_REPEAT:
		if (expr->u.repeat.min > 0) {
			return subtree_contains_anchor(expr->u.repeat.e, type);
		}
		return 0;

	case AST_EXPR_GROUP:
		return subtree_contains_anchor(expr->u.group.e, type);
	case AST_EXPR_SUBTRACT:
		return subtree_contains_anchor(expr->u.subtract.a, type);

	default:
		assert(!"match fail");
	}
}

static void
pincer_anchors_record_anchor(struct pincer_anchors_env *env, struct ast_expr *n)
{
	if (env->used == env->ceil) {
		const size_t nceil = (env->ceil == 0 ? 8 : 2*env->ceil);
		struct ast_expr **nnodes = realloc(env->nodes,
		    nceil * sizeof(nnodes[0]));
		if (nnodes == NULL) {
			env->alloc_fail = 1;
			return;
		}
		env->ceil = nceil;
		env->nodes = nnodes;
	}

	env->nodes[env->used++] = n;
}

/* Check for regexes with out-of-order start and end anchors, combined
 * with alternate subtrees that must consume input. A minimal example of
 * this is `(a|$)(b|^)` (tests/pcre-anchor/in92.re) -- Either an 'a' is
 * consumed or matching must be at the end of input (with optional
 * '\n'), followed by either consuming a 'b' or being at the start of
 * input. This should only match "", "\n", and "<anything>ab<anything>",
 * but because the anchors are out of order it takes a further analysis
 * pass and extra AST_FLAG_CONSTRAINED_AT_{START,END} node flags to
 * ensure that neither is linked to the unanchored start/end loops. */
static void
analysis_iter_pincer_anchors(struct ast_expr *expr, struct pincer_anchors_env *env, enum analysis_direction dir)
{
	LOG(3 - LOG_PINCER_ANCHORS, "%s: expr %p %s\n",
	    __func__, (void *)expr, ast_node_type_name(expr->type));
	switch (expr->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
		return;

	case AST_EXPR_ANCHOR:
		if (expr->u.anchor.type == AST_ANCHOR_START) {
			LOG(3 - LOG_PINCER_ANCHORS, "%s: expr %p ANCHOR_START, flags start %d, end %d, dir %s\n",
			    __func__, (void *)expr,
			    env->always_consumes_or_anchored_start,
			    env->always_consumes_or_anchored_end,
			    dir == AD_FORWARD ? "FORWARD" : "BACKWARD");
			if (dir == AD_FORWARD && env->always_consumes_or_anchored_end) {
				LOG(3 - LOG_PINCER_ANCHORS, "%s: expr %p ANCHOR_START: recording CONSTRAINED_AT_END\n",
				    __func__, (void *)expr);
				/* record for later -- these should only be set if there's an anchored *pair* */
				pincer_anchors_record_anchor(env, expr);
			}
		} else if (expr->u.anchor.type == AST_ANCHOR_END) {
			LOG(3 - LOG_PINCER_ANCHORS, "%s: expr %p ANCHOR_END, flags start %d, end %d, dir %s\n",
			    __func__, (void *)expr,
			    env->always_consumes_or_anchored_start,
			    env->always_consumes_or_anchored_end,
			    dir == AD_FORWARD ? "FORWARD" : "BACKWARD");
			if (dir == AD_BACKWARD && env->always_consumes_or_anchored_start) {
				LOG(3 - LOG_PINCER_ANCHORS, "%s: expr %p ANCHOR_END: recording CONSTRAINED_AT_START\n",
				    __func__, (void *)expr);
				pincer_anchors_record_anchor(env, expr);
			}
		}
		break;

	case AST_EXPR_CONCAT:
		if (dir == AD_FORWARD) {
			for (size_t i = 0; i < expr->u.concat.count; i++) {
				analysis_iter_pincer_anchors(expr->u.concat.n[i], env, dir);
			}
		} else if (dir == AD_BACKWARD) {
			for (size_t i = expr->u.concat.count; i > 0; i--) {
				analysis_iter_pincer_anchors(expr->u.concat.n[i - 1], env, dir);
			}
		}
		break;
	case AST_EXPR_ALT:
	{
		int all_always_consume_or_anchored_start = 1;
		int all_always_consume_or_anchored_end = 1;
		int has_always_consuming = 0;
		int has_non_consuming_anchored_start = 0;
		int has_non_consuming_anchored_end = 0;

		if (dir == AD_FORWARD) {
			for (size_t i = 0; i < expr->u.alt.count; i++) {
				struct ast_expr *child = expr->u.alt.n[i];
				LOG(3 - LOG_PINCER_ANCHORS,
				    "%s: ALT child %zu/%zu : %p\n",
				    __func__, i, expr->u.alt.count, (void *)child);
				if (is_unsatisfiable_or_skipped(child)) { continue; }

				analysis_iter_pincer_anchors(child, env, dir);
				if (child->flags & AST_FLAG_ALWAYS_CONSUMES) {
					has_always_consuming = 1;
				} else {
					if (child->flags & AST_FLAG_ANCHORED_START
					    && subtree_contains_anchor(child, AST_ANCHOR_START)) {
						has_non_consuming_anchored_start = 1;
					} else {
						all_always_consume_or_anchored_start = 0;
					}

					if (child->flags & AST_FLAG_ANCHORED_END
					    && subtree_contains_anchor(child, AST_ANCHOR_END)) {
						has_non_consuming_anchored_end = 1;
					} else {
						all_always_consume_or_anchored_end = 0;
					}
				}
				LOG(3 - LOG_PINCER_ANCHORS,
				    "%s: flags all_acoa_start %d, all_acoa_end %d, hac %d, hnca_start %d, hnca_end %d\n",
				    __func__,
				    all_always_consume_or_anchored_start,
				    all_always_consume_or_anchored_end,
				    has_always_consuming,
				    has_non_consuming_anchored_start,
				    has_non_consuming_anchored_end);
			}
		} else if (dir == AD_BACKWARD) {
			for (size_t i = expr->u.alt.count; i > 0; i--) {
				struct ast_expr *child = expr->u.alt.n[i - 1];
				LOG(3 - LOG_PINCER_ANCHORS,
				    "%s: ALT child %zu/%zu : %p\n",
				    __func__, i, expr->u.alt.count, (void *)child);
				if (is_unsatisfiable_or_skipped(child)) { continue; }

				analysis_iter_pincer_anchors(child, env, dir);
				if (child->flags & AST_FLAG_ALWAYS_CONSUMES) {
					has_always_consuming = 1;
				} else {
					if (child->flags & AST_FLAG_ANCHORED_START
					    && subtree_contains_anchor(child, AST_ANCHOR_START)) {
						has_non_consuming_anchored_start = 1;
					} else {
						all_always_consume_or_anchored_start = 0;
					}

					if (child->flags & AST_FLAG_ANCHORED_END
					    && subtree_contains_anchor(child, AST_ANCHOR_END)) {
						has_non_consuming_anchored_end = 1;
					} else {
						all_always_consume_or_anchored_end = 0;
					}
				}
				LOG(3 - LOG_PINCER_ANCHORS,
				    "%s: flags all_acoa_start %d, all_acoa_end %d, hac %d, hnca_start %d, hnca_end %d\n",
				    __func__,
				    all_always_consume_or_anchored_start,
				    all_always_consume_or_anchored_end,
				    has_always_consuming,
				    has_non_consuming_anchored_start,
				    has_non_consuming_anchored_end);
			}
		}

		if (has_always_consuming
		    && has_non_consuming_anchored_start
		    && all_always_consume_or_anchored_start) {
			LOG(3 - LOG_PINCER_ANCHORS, "%s: ALT %p: setting env->always_consumes_or_anchored_start\n",
			    __func__, (void *)expr);
			env->always_consumes_or_anchored_start = 1;
			LOG(3 - LOG_PINCER_ANCHORS, "%s: %p: setting AST_FLAG_CONSTRAINED_AT_START\n",
			    __func__, (void *)expr);
			set_flags(expr, AST_FLAG_CONSTRAINED_AT_START);
		}
		if (has_always_consuming
		    && has_non_consuming_anchored_end
		    && all_always_consume_or_anchored_end) {
			LOG(3 - LOG_PINCER_ANCHORS, "%s: ALT %p: setting env->always_consumes_or_anchored_end\n",
			    __func__, (void *)expr);
			env->always_consumes_or_anchored_end = 1;
			LOG(3 - LOG_PINCER_ANCHORS, "%s: %p: setting AST_FLAG_CONSTRAINED_AT_END\n",
			    __func__, (void *)expr);
			set_flags(expr, AST_FLAG_CONSTRAINED_AT_END);
		}

		break;
	}

	case AST_EXPR_REPEAT:
		analysis_iter_pincer_anchors(expr->u.repeat.e, env, dir);
		break;

	case AST_EXPR_GROUP:
		analysis_iter_pincer_anchors(expr->u.group.e, env, dir);
		break;
	case AST_EXPR_SUBTRACT:
		/* It could just return here, as long as we aren't
		 * subtracting anything besides character sets. */
		analysis_iter_pincer_anchors(expr->u.subtract.a, env, dir);
		break;

	case AST_EXPR_RANGE:
		/* nothing to do */
		break;

	case AST_EXPR_TOMBSTONE:
		break;
	default:
		assert(!"match fail");
	}
}

static void
analysis_pincer_anchors_apply_flags_iter(struct ast_expr *expr)
{
	LOG(3 - LOG_PINCER_ANCHORS, "%s: expr %p %s\n",
	    __func__, (void *)expr, ast_node_type_name(expr->type));
	switch (expr->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_ANCHOR:
		break;

	case AST_EXPR_CONCAT:
		for (size_t i = 0; i < expr->u.concat.count; i++) {
			analysis_pincer_anchors_apply_flags_iter(expr->u.concat.n[i]);
		}
		break;
	case AST_EXPR_ALT:
		for (size_t i = 0; i < expr->u.alt.count; i++) {
			analysis_pincer_anchors_apply_flags_iter(expr->u.alt.n[i]);
		}
		break;

	case AST_EXPR_REPEAT:
		analysis_pincer_anchors_apply_flags_iter(expr->u.repeat.e);
		break;

	case AST_EXPR_GROUP:
		analysis_pincer_anchors_apply_flags_iter(expr->u.group.e);
		break;

	case AST_EXPR_SUBTRACT:
		/* it could just return here as long as we aren't
		 * subtracting anything besides character sets */
		analysis_pincer_anchors_apply_flags_iter(expr->u.subtract.a);
		analysis_pincer_anchors_apply_flags_iter(expr->u.subtract.b);
		break;

	case AST_EXPR_RANGE:
		/* nothing to do */
		break;

	case AST_EXPR_TOMBSTONE:
		break;
	default:
		assert(!"match fail");
	}
}

static enum ast_analysis_res
analysis_pincer_anchors_apply_flags(struct ast_expr *expr, struct pincer_anchors_env *env)
{
	if (env->alloc_fail) {
		return AST_ANALYSIS_ERROR_MEMORY;
	}

	analysis_pincer_anchors_apply_flags_iter(expr);

	int has_start = 0;
	int has_end = 0;
	for (size_t i = 0; i < env->used; i++) {
		const struct ast_expr *n = env->nodes[i];
		assert(n->type == AST_EXPR_ANCHOR);
		if (n->u.anchor.type == AST_ANCHOR_START) {
			has_start = 1;
		} else if (n->u.anchor.type == AST_ANCHOR_END) {
			has_end = 1;
		}
	}

	if (!has_start || !has_end) {
		/* skip: only meaningful when constrained on both sides */
		return AST_ANALYSIS_OK;
	}

	for (size_t i = 0; i < env->used; i++) {
		struct ast_expr *n = env->nodes[i];
		assert(n->type == AST_EXPR_ANCHOR);
		if (n->u.anchor.type == AST_ANCHOR_START) {
			set_flags(n, AST_FLAG_CONSTRAINED_AT_END);
		} else if (n->u.anchor.type == AST_ANCHOR_END) {
			set_flags(n, AST_FLAG_CONSTRAINED_AT_START);
		}
	}

	return AST_ANALYSIS_OK;
}

enum ast_analysis_res
ast_analysis(struct ast *ast, enum re_flags flags)
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
	 * Next pass: set anchoring, now that nullability info from
	 * the first pass is in place and some other things have been
	 * cleaned up, and note whether the regex has any unsatisfiable
	 * start anchors.
	 */
	{
		/* first anchoring analysis pass, sweeping forward */
		struct anchoring_env env = {
			.re_flags = flags,
			.newline_after_end_anchor_state = NAEAS_NONE,
		};
		res = analysis_iter_anchoring(&env, ast->expr);
		if (res != AST_ANALYSIS_OK) { return res; }

		/* another anchoring analysis pass, sweeping backward */
		res = analysis_iter_reverse_anchoring(&env, ast->expr);
		if (res != AST_ANALYSIS_OK) { return res; }
	}

	/*
	 * Next passes, mark all nodes in a first and/or last
	 * position. This is informed by the anchoring flags, so
	 * that needs to happen first.
	 */
	assign_firsts(ast->expr);
	assign_lasts(ast->expr);

	/* Next pass, mark some cases that need special handling
	 * due to repetition. For example, with cases like
	 * ^((x?))*$ the inner capture will always need to repeat
	 * one more time to match () after any 'x's.
	 *
	 * This needs to happen after the anchoring passes. */
	res = analysis_iter_repetition(ast->expr, NULL, 0, NULL);
	if (res != AST_ANALYSIS_OK) { return res; }

	/* Based on all analysis thus far, check for another obscure
	 * edge case that is otherwise difficult to handle properly.
	 * This may set an extra flag on start/end anchor nodes. */
	{
		struct pincer_anchors_env pincer_anchors_env = { .ceil = 0, };
		LOG(3 - LOG_PINCER_ANCHORS, "%s: -> analysis_iter_pincer_anchors FORWARD\n", __func__);
		analysis_iter_pincer_anchors(ast->expr, &pincer_anchors_env, AD_FORWARD);

		LOG(3 - LOG_PINCER_ANCHORS, "%s: -> analysis_iter_pincer_anchors BACKWARD\n", __func__);
		pincer_anchors_env.always_consumes_or_anchored_start = 0;
		pincer_anchors_env.always_consumes_or_anchored_end = 0;
		analysis_iter_pincer_anchors(ast->expr, &pincer_anchors_env, AD_BACKWARD);

		res = analysis_pincer_anchors_apply_flags(ast->expr, &pincer_anchors_env);
		free(pincer_anchors_env.nodes);

		if (res != AST_ANALYSIS_OK) {
			return res;
		}
	}

	ast->has_unanchored_start = (analysis_iter_unanchored_start(ast->expr) != UA_NO);
	ast->has_unanchored_end = (analysis_iter_unanchored_end(ast->expr) != UA_NO);
	LOG(2 - LOG_UNANCHORED_FLAGS,
	    "%s: has_unanchored_start %d, has_unanchored_end %d\n",
	    __func__, ast->has_unanchored_start, ast->has_unanchored_end);

	return res;
}
