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

#define LOG(LEVEL, ...)							\
	do {								\
		if ((LEVEL) <= LOG_ANALYSIS) {				\
			fprintf(stderr, __VA_ARGS__);			\
		}							\
	} while(0)

/* Mask for end-anchor flags */
#define END_ANCHOR_FLAG_MASK (AST_FLAG_ANCHORED_END | AST_FLAG_END_NL)

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
			analysis_iter(child);
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
			analysis_iter(child);

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

		analysis_iter(e);
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

		analysis_iter(e);
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

struct anchoring_env {
	enum re_flags re_flags;

	/* When sweeping forward, is the node being evaluated past
	 * a point where input must have been consumed? This is used
	 * to mark start anchors as unsatisfiable. */
	int past_always_consuming;

	/* Corresponding flag for end anchors while sweeping backward. */
	int followed_by_consuming;

	int before_start_anchor;
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
			if (n->u.anchor.is_end_nl && !(env->re_flags & RE_ANCHORED)) {
				set_flags(n, AST_FLAG_END_NL);
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
					child->u.repeat.max = 0;
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
				all_set_past_always_consuming &= child_env.past_always_consuming;
				any_sat = 1;
			} else {
				return res;
			}

			if (!(child->flags & AST_FLAG_UNSATISFIABLE)) { /* ignore unsat nodes */
				if (!(child->flags & AST_FLAG_ANCHORED_START)) {
					all_start_anchored = 0;
				}
				if (!(child->flags & AST_FLAG_ANCHORED_END)) {
					all_end_anchored = 0;
				}
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

	case AST_EXPR_REPEAT:
		res = analysis_iter_anchoring(env, n->u.repeat.e);

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
				n->u.repeat.max = 0;
				break;
			} else if (n->u.repeat.min > 0) {
				set_flags(n, AST_FLAG_UNSATISFIABLE);
				LOG(3 - LOG_ANCHORING,
				    "%s: REPEAT: UNSATISFIABLE and must be repeated >0 times -> UNSATISFIABLE\n", __func__);
				return AST_ANALYSIS_UNSATISFIABLE;
			}
		} else if (res != AST_ANALYSIS_OK) {
			LOG(3 - LOG_ANCHORING,
			    "%s: REPEAT: analysis on child returned %d\n", __func__, res);
			return res;
		}

		if (n->u.repeat.e->flags & AST_FLAG_ANCHORED_END && n->u.repeat.min > 0) {
			/* FIXME: if repeating something that is always
			 * anchored at the end, repeat.max could be
			 * capped at 1, but I have not yet found any
			 * inputs where that change is necessary to
			 * produce a correct result. */
			LOG(3 - LOG_ANCHORING,
			    "%s: REPEAT: repeating ANCHORED_END subtree >0 times -> ANCHORED_END\n", __func__);
			set_flags(n, n->u.repeat.e->flags & END_ANCHOR_FLAG_MASK);
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
		break;

	case AST_EXPR_SUBTRACT:
		res = analysis_iter_anchoring(env, n->u.subtract.a);
		if (n->u.subtract.a->flags & AST_FLAG_ANCHORED_START) {
			set_flags(n, AST_FLAG_ANCHORED_START);
		}
		if (n->u.subtract.a->flags & AST_FLAG_ANCHORED_END) {
			set_flags(n, n->u.subtract.a->flags & END_ANCHOR_FLAG_MASK);
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
			/* should already be set during forward pass */
			assert(n->flags & AST_FLAG_ANCHORED_END);

			if (env->followed_by_consuming) {
				LOG(3 - LOG_ANCHORING,
				    "%s: END anchor & followed_by_consuming, setting UNSATISFIABLE\n",
				    __func__);
				set_flags(n, AST_FLAG_UNSATISFIABLE);
				return AST_ANALYSIS_UNSATISFIABLE;
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
				return res;
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
				all_set_followed_by_consuming &= child_env.followed_by_consuming;
				all_set_before_start_anchor &= child_env.before_start_anchor;
				any_sat = 1;
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

		if (!env->before_start_anchor && all_set_before_start_anchor) {
			LOG(3 - LOG_ANCHORING,
			    "%s: ALT: all_set_before_start_anchor -> setting env->before_start_anchor\n",
			    __func__);
			env->before_start_anchor = 1;
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
		res = analysis_iter_reverse_anchoring(env, n->u.repeat.e);
		if (res == AST_ANALYSIS_UNSATISFIABLE) {
			if (n->u.repeat.min == 0) {
				LOG(3 - LOG_ANCHORING,
				    "%s: REPEAT: UNSATISFIABLE but can be repeated 0 times, ignoring\n", __func__);
				n->u.repeat.max = 0; /* skip */
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
		if (n->u.repeat.max > 0) {
			set_flags(n, AST_FLAG_FIRST);
		}

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

	case AST_EXPR_REPEAT:
		if (n->u.repeat.max > 0) {
			set_flags(n, AST_FLAG_LAST);
		}

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
		struct anchoring_env env = { .re_flags = flags };
		res = analysis_iter_anchoring(&env, ast->expr);
		if (res != AST_ANALYSIS_OK) { return res; }

		/* another anchoring analysis pass, sweeping backward */
		res = analysis_iter_reverse_anchoring(&env, ast->expr);
		if (res != AST_ANALYSIS_OK) { return res; }

		/*
		 * Next passes, mark all nodes in a first and/or last
		 * position. This is informed by the anchoring flags, so
		 * that needs to happen first.
		 */
		assign_firsts(ast->expr);
		assign_lasts(ast->expr);

		ast->has_unanchored_start = (analysis_iter_unanchored_start(ast->expr) != UA_NO);
		ast->has_unanchored_end = (analysis_iter_unanchored_end(ast->expr) != UA_NO);
		LOG(2 - LOG_UNANCHORED_FLAGS,
		    "%s: has_unanchored_start %d, has_unanchored_end %d\n",
		    __func__, ast->has_unanchored_start, ast->has_unanchored_end);
	}

	return res;
}
