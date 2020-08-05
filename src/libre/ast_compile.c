/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/capture.h>

#include <re/re.h>
#include <re/strings.h>

#include "class.h"
#include "ast.h"
#include "ast_compile.h"

#include "libfsm/internal.h" /* XXX */

/*
 * Aho-Corasick requires constructing a trie, and that has its own overhead.
 * It's only worth doing this if the cost overall is lower.
 *
 * The parameters here bail out if the number of alts or the length of any
 * particular string within an alt are below a threshold, especially because
 * [xyz] is so common. The exact values only represent cost approximately.
 *
 * In practice I found it difficult to pick values for these, because for
 * the lower end of the scale it's hard to see a meaningful difference,
 * and for the upper end of the scale the time is dominated by
 * determinisation anyway.
 */
#define AC_COUNT_THRESHOLD  5
#define AC_LENGTH_THRESHOLD 5

#define LOG_LINKAGE 0

enum link_side {
	LINK_START,
	LINK_END
};

/*
 * How should this state be linked for the relevant state?
 * Note: These are not mutually exclusive!
 *
 * - LINK_TOP_DOWN
 *   Use the passed in start/end states (x and y)
 *
 * - LINK_GLOBAL
 *   Link to the global start/end state (env->start or env->end),
 *   because this node has a ^ or $ anchor
 *
 * - LINK_GLOBAL_SELF_LOOP
 *   Link to the unanchored self loop adjacent to the start/end
 *   state (env->start_any_loop or env->end_any_loop), because
 *   this node is in a FIRST or LAST position, but unanchored.
 */
enum link_types {
	LINK_TOP_DOWN         = 1 << 0,
	LINK_GLOBAL           = 1 << 1,
	LINK_GLOBAL_SELF_LOOP = 1 << 2,

	LINK_NONE = 0x00
};

struct comp_env {
	struct fsm *fsm;
	enum re_flags re_flags;
	struct re_err *err;

	/*
	 * These are saved so that dialects without implicit
	 * anchoring can create states with '.' edges to self
	 * on demand, and link them to the original start and
	 * end states.
	 *
	 * Also, some states in a first/last context need to link
	 * directly to the overall start/end states, either in
	 * place of or along with the adjacent states.
	 */
	fsm_state_t start;
	fsm_state_t end;
	fsm_state_t start_any_loop;
	fsm_state_t end_any_loop;
	int have_start_any_loop;
	int have_end_any_loop;
};

static int
ast_compile_expr(struct comp_env *env,
	fsm_state_t x, fsm_state_t y,
	const struct ast_expr *n);

static int
utf8(uint32_t cp, char c[])
{
	if (cp <= 0x7f) {
		c[0] =  cp;
		return 1;
	}

	if (cp <= 0x7ff) {
		c[0] = (cp >>  6) + 192;
		c[1] = (cp  & 63) + 128;
		return 2;
	}

	if (0xd800 <= cp && cp <= 0xdfff) {
		/* invalid */
		goto error;
	}

	if (cp <= 0xffff) {
		c[0] =  (cp >> 12) + 224;
		c[1] = ((cp >>  6) &  63) + 128;
		c[2] =  (cp  & 63) + 128;
		return 3;
	}

	if (cp <= 0x10ffff) {
		c[0] =  (cp >> 18) + 240;
		c[1] = ((cp >> 12) &  63) + 128;
		c[2] = ((cp >>  6) &  63) + 128;
		c[3] =  (cp  & 63) + 128;
		return 4;
	}

error:

	return 0;
}

/* TODO: centralise as fsm_unionxy() perhaps */
static struct fsm *
fsm_unionxy(struct fsm *a, struct fsm *b, fsm_state_t *sa, fsm_state_t *sb,
	fsm_state_t x, fsm_state_t y)
{
	struct fsm *q;
	fsm_state_t end;
	fsm_state_t base_b;

	assert(a != NULL);
	assert(b != NULL);
	assert(sa != NULL);
	assert(sb != NULL);

	/* x,y both belong to a */
	assert(x < a->statecount);
	assert(y < a->statecount);

	if (!fsm_collate(b, &end, fsm_isend)) {
		return NULL;
	}

	/* TODO: centralise as fsm_clearends() or somesuch */
	{
		size_t i;

		for (i = 0; i < b->statecount; i++) {
			fsm_setend(b, i, 0);
		}
	}

	q = fsm_mergeab(a, b, &base_b);
	if (q == NULL) {
		return NULL;
	}

	*sb += base_b;
	end += base_b;

	if (!fsm_addedge_epsilon(q, x, *sb)) {
		return NULL;
	}

	if (!fsm_addedge_epsilon(q, end, y)) {
		return NULL;
	}

	return q;
}

static int
is_dotstar(const struct ast_expr *n)
{
	assert(n != NULL);

	if (n->type != AST_EXPR_REPEAT) {
		return 0;
	}

	if (n->u.repeat.min != 0 || n->u.repeat.max != AST_COUNT_UNBOUNDED) {
		return 0;
	}

	if (n->u.repeat.e->type != AST_EXPR_ANY) {
		return 0;
	}

	return 1;
}

static int
is_ac_candidate(const struct ast_expr *n, enum re_strings_flags *flags,
	size_t *o_out, size_t *l_out)
{
	size_t o, count;
	size_t i;

	assert(n != NULL);
	assert(flags != NULL);

	o = 0;
	count = n->u.concat.count;

	/*
	 * We're looking at a single literal in an alt: /...|x|.../
	 * We could use AC here and treat this as a string anchored at both ends,
	 * but the interface here deals with offsets into the array of children
	 * for a concat node only. It's probably also overkill to involve AC for
	 * alts of single literals. So here we defer to the usual NFA construction.
	 */
	if (n->type == AST_EXPR_LITERAL) {
		return 0;
	}

	/*
	 * We're looking at some other kind of node in an alt: /...|x+|.../
	 * where we wouldn't be able to use AC anyway.
	 */
	if (n->type != AST_EXPR_CONCAT) {
		return 0;
	}

	/*
	 * This should never happen; a single-node concat is optimised away.
	 */
	if (n->u.alt.count == 1) {
		return is_ac_candidate(n->u.alt.n[0], flags, o_out, l_out);
	}

	/*
	 * The general form here is an n-ary concat like [.*] a b c [.*]
	 * where .* is optional at either end. We're detecting the presence
	 * of those, moving the offset and length to skip them, and setting
	 * the appropriate RE_STRINGS_ANCHOR_LEFT/_RIGHT flags.
	 *
	 * "Anchor" here (as far as our implementation of Aho-Corasick cares) means
	 * the absence of .* at either end; this is not the same as the ^$ anchors
	 * in regexp syntax (because we're in the middle of an AST here).
	 */
	*flags = RE_STRINGS_ANCHOR_LEFT | RE_STRINGS_ANCHOR_RIGHT;

	if (count >= 1 && is_dotstar(n->u.concat.n[o])) {
		o++;
		count--;
		*flags &= ~RE_STRINGS_ANCHOR_LEFT;
	}

	if (count >= 1 && is_dotstar(n->u.concat.n[count + o - 1])) {
		count--;
		*flags &= ~RE_STRINGS_ANCHOR_RIGHT;
	}

	if (count == 0) {
		return 0;
	}

	/*
	 * We also validate that the middle part contains a run of literals only,
	 * else we're not suitable for AC anyway.
	 */
	for (i = o; i < count; i++) {
		if (n->u.concat.n[i]->type != AST_EXPR_LITERAL) {
			return 0;
		}
	}

	*o_out = o;
	*l_out = count;

	return 1;
}

static struct fsm *
expr_compile(struct ast_expr *e, enum re_flags flags,
	const struct fsm_options *opt, struct re_err *err)
{
	struct fsm *fsm;
	struct ast ast;
	fsm_state_t start;

	ast.expr = e;

	fsm = fsm_new(opt);
	if (fsm == NULL) {
		return NULL;
	}

	if (!ast_compile(&ast, fsm, &start, flags, err)) {
		return NULL;
	}

	fsm_setstart(fsm, start);

	return fsm;
}

static int
addedge_literal(struct comp_env *env,
	fsm_state_t from, fsm_state_t to, char c)
{
	struct fsm *fsm = env->fsm;
	assert(fsm != NULL);

	assert(from < env->fsm->statecount);
	assert(to < env->fsm->statecount);

	if (env->re_flags & RE_ICASE) {
		if (!fsm_addedge_literal(fsm, from, to, tolower((unsigned char) c))) {
			return 0;
		}
		
		if (!fsm_addedge_literal(fsm, from, to, toupper((unsigned char) c))) {
			return 0;
		}
	} else {
		if (!fsm_addedge_literal(fsm, from, to, c)) {
			return 0;
		}
	}
	
	return 1;
}

static int
intern_start_any_loop(struct comp_env *env)
{
	fsm_state_t loop;

	assert(env != NULL);

	if (env->have_start_any_loop) {
		return 1;
	}

	assert(~env->re_flags & RE_ANCHORED);
	assert(env->start < env->fsm->statecount);

	if (!fsm_addstate(env->fsm, &loop)) {
		return 0;
	}

	if (!fsm_addedge_any(env->fsm, loop, loop)) {
		return 0;
	}

	if (!fsm_addedge_epsilon(env->fsm, env->start, loop)) {
		return 0;
	}

	env->start_any_loop = loop;
	env->have_start_any_loop = 1;

	return 1;
}

static int
intern_end_any_loop(struct comp_env *env)
{
	fsm_state_t loop;

	assert(env != NULL);

	if (env->have_end_any_loop) {
		return 1;
	}

	assert(~env->re_flags & RE_ANCHORED);
	assert(env->end < env->fsm->statecount);

	if (!fsm_addstate(env->fsm, &loop)) {
		return 0;
	}

	if (!fsm_addedge_any(env->fsm, loop, loop)) {
		return 0;
	}

	if (!fsm_addedge_epsilon(env->fsm, loop, env->end)) {
		return 0;
	}

	env->end_any_loop = loop;
	env->have_end_any_loop = 1;

	return 1;
}

static int
can_have_backward_epsilon_edge(const struct ast_expr *e)
{
	switch (e->type) {
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_FLAGS:
	case AST_EXPR_ALT:
	case AST_EXPR_ANCHOR:
	case AST_EXPR_RANGE:
		/* These nodes cannot have a backward epsilon edge */
		return 0;

	case AST_EXPR_SUBTRACT:
		/* XXX: not sure */
		return 1;

	case AST_EXPR_REPEAT:
		/* 0 and 1 don't have backward epsilon edges */
		if (e->u.repeat.max <= 1) {
			return 0;
		}

		/*
		 * The general case for counted repetition already
		 * allocates one-way guard states around it
		 */
		if (e->u.repeat.max != AST_COUNT_UNBOUNDED) {
			return 0;
		}

		return 1;

	case AST_EXPR_GROUP:
		return can_have_backward_epsilon_edge(e->u.group.e);

	default:
		break;
	}

	return 1;
}

static int
can_skip_concat_state_and_epsilon(const struct ast_expr *l,
	const struct ast_expr *r)
{
	assert(l != NULL);

	/*
	 * CONCAT only needs the extra state and epsilon edge when there
	 * is a backward epsilon edge for repetition - without it, a
	 * regex such as /a*b*c/ could match "ababc" as well as "aabbc",
	 * because the backward epsilon for repeating the 'b' would lead
	 * to a state which has another backward epsilon for repeating
	 * the 'a'. The extra state functions as a one-way guard,
	 * keeping the match from looping further back in the FSM than
	 * intended.
	 */

	if (!can_have_backward_epsilon_edge(l)) {
		return 1;
	}

	if (r != NULL && r->type == AST_EXPR_REPEAT) {
		if (!can_have_backward_epsilon_edge(r)) {
			return 1;
		}
	}

	return 0;
}

static enum link_types
decide_linking(struct comp_env *env,
	fsm_state_t x, fsm_state_t y,
	const struct ast_expr *n, enum link_side side)
{
	enum link_types res = LINK_NONE;

	assert(n != NULL);
	assert(env != NULL);

	if ((env->re_flags & RE_ANCHORED)) {
		return LINK_TOP_DOWN;
	}

	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_GROUP:
		return LINK_TOP_DOWN;

	case AST_EXPR_ANCHOR:
		if (n->u.anchor.type == AST_ANCHOR_START && side == LINK_START) {
			return LINK_GLOBAL;
		}
		if (n->u.anchor.type == AST_ANCHOR_END && side == LINK_END) {
			return LINK_GLOBAL;
		}

		break;

	case AST_EXPR_SUBTRACT:
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_ANY:

	case AST_EXPR_CONCAT:
	case AST_EXPR_ALT:
	case AST_EXPR_REPEAT:
	case AST_EXPR_FLAGS:
	case AST_EXPR_RANGE:
	case AST_EXPR_TOMBSTONE:
		break;

	default:
		assert(!"unreached");
	}

	switch (side) {
	case LINK_START: {
		const int start    = (n->type == AST_EXPR_ANCHOR && n->u.anchor.type == AST_ANCHOR_START);
		const int first    = (n->flags & AST_FLAG_FIRST) != 0;
		const int nullable = (n->flags & AST_FLAG_NULLABLE) != 0;

		(void) nullable;

		if (!start && first) {
			if (x == env->start) {
				/* Avoid a cycle back to env->start that may
				 * lead to incorrect matches, e.g. /a?^b*/
				return LINK_GLOBAL_SELF_LOOP;
			} else {
				/* Link in the starting self-loop, but also the
				 * previous state (if any), because it can
				 * indicate matching a nullable state. */
				return LINK_GLOBAL_SELF_LOOP | LINK_TOP_DOWN;
			}
		}

		if (start && !first) {
			return LINK_GLOBAL;
		}

		return LINK_TOP_DOWN;
	}

	case LINK_END: {
		const int end      = (n->type == AST_EXPR_ANCHOR && n->u.anchor.type == AST_ANCHOR_END);
		const int last     = (n->flags & AST_FLAG_LAST) != 0;
		const int nullable = (n->flags & AST_FLAG_NULLABLE) != 0;

		(void) nullable;

		if (end && last) {
			if (y == env->end) {
				return LINK_GLOBAL;
			} else {
				return LINK_GLOBAL | LINK_TOP_DOWN;
			}
		}

		if (!end && last) {
			if (y == env->end) {
				return LINK_GLOBAL_SELF_LOOP;
			} else {
				return LINK_GLOBAL_SELF_LOOP | LINK_TOP_DOWN;
			}
		}

		if (end && !last) {
			return LINK_GLOBAL;
		}

		return LINK_TOP_DOWN;
	}

	default:
		assert(!"unreached");
	}

	assert(res != LINK_NONE);

	return res;
}

static void
print_linkage(enum link_types t)
{
	if (t == LINK_NONE) {
		fprintf(stderr, "NONE");
		return;
	}

	if (t & LINK_TOP_DOWN) {
		fprintf(stderr, "[TOP_DOWN]");
	}
	if (t & LINK_GLOBAL) {
		fprintf(stderr, "[GLOBAL]");
	}
	if (t & LINK_GLOBAL_SELF_LOOP) {
		fprintf(stderr, "[SELF_LOOP]");
	}
}

#define NEWSTATE(NAME)              \
    if (!fsm_addstate(env->fsm, &(NAME))) { return 0; }

#define EPSILON(FROM, TO)           \
    assert((FROM) != (TO));         \
    if (!fsm_addedge_epsilon(env->fsm, (FROM), (TO))) { return 0; }
        
#define ANY(FROM, TO)               \
    if (!fsm_addedge_any(env->fsm, (FROM), (TO))) { return 0; }

#define LITERAL(FROM, TO, C)        \
    if (!addedge_literal(env, (FROM), (TO), (C))) { return 0; }

#define RECURSE(FROM, TO, NODE)     \
    if (!ast_compile_expr(env, (FROM), (TO), (NODE))) { return 0; }

static int
ast_compile_repeat(struct comp_env *env,
	fsm_state_t x, fsm_state_t y,
	const struct ast_expr_repeat *n)
{
	fsm_state_t a, b;
	fsm_state_t na, nz;
	unsigned i, min, max;

	min = n->min;
	max = n->max;

	assert(min <= max);

	if (min == 0 && max == 0) {                          /* {0,0} */
		EPSILON(x, y);
	} else if (min == 0 && max == 1) {                   /* '?' */
		RECURSE(x, y, n->e);
		EPSILON(x, y);
	} else if (min == 1 && max == 1) {                   /* {1,1} */
		RECURSE(x, y, n->e);
	} else if (min == 0 && max == AST_COUNT_UNBOUNDED) { /* '*' */
		NEWSTATE(na);
		NEWSTATE(nz);
		EPSILON(x,na);
		EPSILON(nz,y);

		EPSILON(na, nz);
		RECURSE(na, nz, n->e);
		EPSILON(nz, na);
	} else if (min == 1 && max == AST_COUNT_UNBOUNDED) { /* '+' */
		NEWSTATE(na);
		NEWSTATE(nz);
		EPSILON(x,na);
		EPSILON(nz,y);

		RECURSE(na, nz, n->e);
		EPSILON(nz, na);
	} else {
		/*
		 * Make new beginning/end states for the repeated section,
		 * build its NFA, and link to its head.
		 */

		struct fsm_capture capture;
		fsm_state_t tail;

		fsm_capture_start(env->fsm, &capture);

		NEWSTATE(na);
		NEWSTATE(nz);
		RECURSE(na, nz, n->e);
		EPSILON(x, na); /* link head to repeated NFA head */

		b = nz; /* set the initial tail */

		/* can be skipped */
		if (min == 0) {
			EPSILON(na, nz);
		}
		fsm_capture_stop(env->fsm, &capture);
		tail = nz;

		if (max != AST_COUNT_UNBOUNDED) {
			for (i = 1; i < max; i++) {
				/* copies the original subgraph; need to set b to the
				 * original tail
				 */
				b = tail;

				if (!fsm_capture_duplicate(env->fsm, &capture, &b, &a)) {
					return 0;
				}

				EPSILON(nz, a);

				/* To the optional part of the repeated count */
				if (i >= min) {
					EPSILON(nz, b);
				}

				na = a;	/* advance head for next duplication */
				nz = b;	/* advance tail for concenation */
			}
		} else {
			for (i = 1; i < min; i++) {
				/* copies the original subgraph; need to set b to the
				 * original tail
				 */
				b = tail;

				if (!fsm_capture_duplicate(env->fsm, &capture, &b, &a)) {
					return 0;
				}

				EPSILON(nz, a);

				na = a;	/* advance head for next duplication */
				nz = b;	/* advance tail for concenation */
			}

			/* back link to allow for infinite repetition */
			EPSILON(nz,na);
		}

		/* tail to last repeated NFA tail */
		EPSILON(nz, y);
	}

	return 1;
}

static int
ast_compile_altlist(struct comp_env *env,
	fsm_state_t x, int have_end, fsm_state_t y,
	const struct ast_expr **n, size_t count)
{
	struct re_strings *a[] = { NULL, NULL, NULL, NULL };
	size_t i;

	assert(count >= 1);

	for (i = 0; i < count; i++) {
		enum re_strings_flags flags;
		size_t o, l;

		if (count < AC_COUNT_THRESHOLD) {
			RECURSE(x, y, n[i]);
			continue;
		}

		if (n[i]->type != AST_EXPR_CONCAT ||
			n[i]->u.concat.count < AC_LENGTH_THRESHOLD)
		{
			RECURSE(x, y, n[i]);
			continue;
		}

		if (!is_ac_candidate(n[i], &flags, &o, &l)) {
			RECURSE(x, y, n[i]);
			continue;
		}

		if (a[flags] == NULL) {
			a[flags] = re_strings_new();
			if (a[flags] == NULL) {
				return 0;
			}
		}

		/* XXX: i'm screwing up the const handling here, i'm sure */
		if (!re_strings_add_concat(a[flags],
			(const struct ast_expr **) n[i]->u.concat.n + o, l))
		{
			return 0;
		}
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fsm_state_t start;

		if (a[i] == NULL) {
			continue;
		}

		if (!re_strings_build_into(env->fsm, &start, have_end, y, a[i], i)) {
			return 0;
		}
		/* XXX: would love to avoid the epsilon here, for non-dotstar ac,
		 * maybe we could pass in x directly? */
		EPSILON(x, start);
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		re_strings_free(a[i]);
	}

	return 1;
}

static int
ast_compile_expr(struct comp_env *env,
	fsm_state_t x, fsm_state_t y,
	const struct ast_expr *n)
{
	enum link_types link_start, link_end;

	if (n == NULL) {
		return 1;
	}

	link_start = decide_linking(env, x, y, n, LINK_START);
	link_end   = decide_linking(env, x, y, n, LINK_END);

#if LOG_LINKAGE
	fprintf(stderr, "%s: decide_linking %p: start ", __func__, (void *) n);
	print_linkage(link_start);
	fprintf(stderr, ", end ");
	print_linkage(link_end);
	fprintf(stderr, "\n");
#else
	(void) print_linkage;
#endif

	if ((link_start & LINK_TOP_DOWN) == LINK_NONE) {
		/*
		 * The top-down link is rejected, so replace x with
		 * either the NFA's global start state or the self-loop
		 * at the start. These _are_ mutually exclusive.
		 */
		if (link_start & LINK_GLOBAL) {
			assert((link_start & LINK_GLOBAL_SELF_LOOP) == LINK_NONE);
			x = env->start;
		} else if (link_start & LINK_GLOBAL_SELF_LOOP) {
			assert((link_start & LINK_GLOBAL) == LINK_NONE);

			if (!intern_start_any_loop(env)) {
				return 0;
			}

			assert(env->have_start_any_loop);
			x = env->start_any_loop;
		}
	} else {
		/*
		 * The top-down link is still being used, so connect to the
		 * global start/start-self-loop state with an epsilon.
		 */
		if (link_start & LINK_GLOBAL) {
			assert((link_start & LINK_GLOBAL_SELF_LOOP) == LINK_NONE);
			EPSILON(env->start, x);
		} else if (link_start & LINK_GLOBAL_SELF_LOOP) {
			assert((link_start & LINK_GLOBAL) == LINK_NONE);

			if (!intern_start_any_loop(env)) {
				return 0;
			}

			assert(env->have_start_any_loop);
		}
	}

	if ((link_end & LINK_TOP_DOWN) == LINK_NONE) {
		/*
		 * The top-down link is rejected, so replace x with
		 * either the NFA's global end state or the self-loop
		 * at the end. These _are_ mutually exclusive.
		 */
		if (link_end & LINK_GLOBAL) {
			assert((link_end & LINK_GLOBAL_SELF_LOOP) == LINK_NONE);
			y = env->end;
		} else if (link_end & LINK_GLOBAL_SELF_LOOP) {
			assert((link_end & LINK_GLOBAL) == LINK_NONE);

			if (!intern_end_any_loop(env)) {
				return 0;
			}

			assert(env->have_end_any_loop);
			y = env->end_any_loop;
		}
	} else {
		/*
		 * The top-down link is still being used, so connect to the
		 * global end/end-self-loop state with an epsilon.
		 */
		if (link_end & LINK_GLOBAL) {
			assert((link_end & LINK_GLOBAL_SELF_LOOP) == LINK_NONE);
			EPSILON(y, env->end);
		} else if (link_end & LINK_GLOBAL_SELF_LOOP) {
			assert((link_end & LINK_GLOBAL) == LINK_NONE);

			if (!intern_end_any_loop(env)) {
				return 0;
			}

			assert(env->have_end_any_loop);
		}
	}

	switch (n->type) {
	case AST_EXPR_EMPTY:
		/* skip these, when possible */
		EPSILON(x, y);
		break;

	case AST_EXPR_CONCAT: {
		fsm_state_t base, z;
		fsm_state_t curr_x;
		enum re_flags saved;
		size_t i;

		const size_t count  = n->u.concat.count;

		curr_x = x;
		saved  = env->re_flags;

		assert(count >= 1);

		base = env->fsm->statecount;

		if (!fsm_addstate_bulk(env->fsm, count - 1)) {
			return 0;
		}

		for (i = 0; i < count; i++) {
			const struct ast_expr *curr = n->u.concat.n[i];
			const struct ast_expr *next = i == count - 1
				? NULL
				: n->u.concat.n[i + 1];

			if (i + 1 < count) {
				z = base + i;
			} else {
				z = y;
			}

			if (curr->type == AST_EXPR_FLAGS) {
				/*
				 * Save the current flags in the flags node,
				 * restore when done evaluating the concat
				 * node's right subtree.
				 */
				saved = env->re_flags;
				
				/*
				 * Note: in cases like `(?i-i)`, the negative is
				 * required to take precedence.
				 */
				env->re_flags |=  curr->u.flags.pos;
				env->re_flags &= ~curr->u.flags.neg;
			}

			/*
			 * If nullable, add an extra state & epsilion as a one-way gate
			 */
			if (!can_skip_concat_state_and_epsilon(curr, next)) {
				fsm_state_t diode;

				NEWSTATE(diode);
				EPSILON(curr_x, diode);
				curr_x = diode;
			}

			RECURSE(curr_x, z, curr);

			curr_x = z;
		}

		env->re_flags = saved;

		break;
	}

	case AST_EXPR_ALT:
		if (!ast_compile_altlist(env, x, 1, y, (const struct ast_expr **) n->u.alt.n, n->u.alt.count)) {
			return 0;
		}
		break;

	case AST_EXPR_LITERAL:
		LITERAL(x, y, n->u.literal.c);
		break;

	case AST_EXPR_CODEPOINT: {
		fsm_state_t a, b;
		char c[4];
		int r, i;

		r = utf8(n->u.codepoint.u, c);
		if (!r) {
			if (env->err != NULL) {
				env->err->e = RE_EBADCP;
				env->err->cp = n->u.codepoint.u;
			}

			return 0;
		}

		a = x;

		for (i = 0; i < r; i++) {
			if (i + 1 < r) {
				NEWSTATE(b);
			} else {
				b = y;
			}

			LITERAL(a, b, c[i]);

			a = b;
		}

		break;
	}

	case AST_EXPR_ANY:
		ANY(x, y);
		break;

	case AST_EXPR_REPEAT:
		if (!ast_compile_repeat(env, x, y, &n->u.repeat)) {
			return 0;
		}
		break;

	case AST_EXPR_GROUP:
		RECURSE(x, y, n->u.group.e);
		break;

	case AST_EXPR_FLAGS:
		/*
		 * This is purely a metadata node, handled at analysis
		 * time; just bridge the start and end states.
		 */
		EPSILON(x, y);

	case AST_EXPR_TOMBSTONE:
		/* do not link -- intentionally pruned */
		break;

	case AST_EXPR_ANCHOR:
		switch (n->u.anchor.type) {
		case AST_ANCHOR_START:
			EPSILON(env->start, y);
			break;

		case AST_ANCHOR_END:
			EPSILON(x, env->end);
			break;

		default:
			assert(!"unreached");
		}
		break;

	case AST_EXPR_SUBTRACT: {
		struct fsm *a, *b;
		struct fsm *q;
		enum re_flags re_flags;

		re_flags = env->re_flags;

		/* wouldn't want to reverse twice! */
		re_flags &= ~RE_REVERSE;

		a = expr_compile(n->u.subtract.a, re_flags,
			fsm_getoptions(env->fsm), env->err);
		if (a == NULL) {
			return 0;
		}

		b = expr_compile(n->u.subtract.b, re_flags,
			fsm_getoptions(env->fsm), env->err);
		if (b == NULL) {
			fsm_free(a);
			return 0;
		}

		q = fsm_subtract(a, b);
		if (q == NULL) {
			return 0;
		}

		/*
		 * Subtraction produces quite a mess. We could trim or minimise here
		 * while q is self-contained, which might work out better than doing it
		 * in the larger FSM after merge. I'm not sure if it works out better
		 * overall or not.
		 */

		if (fsm_empty(q)) {
			EPSILON(x, y);
			break;
		}

		{
			fsm_state_t sb;
			struct fsm *z;

			if (!fsm_getstart(q, &sb)) {
				return 0;
			}

			z = fsm_unionxy(env->fsm, q, &env->start, &sb, x, y);
			if (z == NULL) {
				return 0;
			}

			(void) z;
		}

		break;
	}

	case AST_EXPR_RANGE: {
		unsigned int i;

		if (n->u.range.from.type != AST_ENDPOINT_LITERAL || n->u.range.to.type != AST_ENDPOINT_LITERAL) {
			/* not yet supported */
			return 0;
		}

		assert(n->u.range.from.u.literal.c <= n->u.range.to.u.literal.c);

		for (i = n->u.range.from.u.literal.c; i <= n->u.range.to.u.literal.c; i++) {
			LITERAL(x, y, i);
		}

		break;
	}

	default:
		assert(!"unreached");
	}

	return 1;
}

int
ast_compile_root(struct comp_env *env,
	fsm_state_t x, fsm_state_t y,
	const struct ast_expr *n)
{
	assert(env != NULL);
	assert(n != NULL);

	/*
	 * The root node is handled specially when it's suitable for Aho-Corasick,
	 * because then we can construct a trie with accepting states in-situ along
	 * the branches, instead of hooking them up with epsilons to the y state.
	 * This reduces pressure on resolving those epsilons later on.
	 *
	 * To do this, we pass have_end=0 so that re_strings_build_into() does not
	 * use the shared end state we would normally use during the recursive
	 * Thompson NFA construction.
	 *
	 * For things which aren't suitable for Aho-Corasick, recursion will
	 * continue per usual, constructed alongside the trie (if present at all).
	 */

	/*
	 * Groups have no relevance on the structure being suitable for A-C,
	 * so we recurr into those.
	 */
	if (n->type == AST_EXPR_GROUP) {
		return ast_compile_root(env, x, y, n->u.group.e);
	}

	if (env->re_flags & RE_ANCHORED && n->type == AST_EXPR_ALT) {
		return ast_compile_altlist(env, x, 0, y,
			(const struct ast_expr **) n->u.alt.n, n->u.alt.count);
	}

	/* XXX: this leaves a stray end state for y when we only have a trie, would prefer to avoid that */
	/* TODO: deal with ~RE_ANCHORED */
	/* TODO: special cases for ^...$ alts, too */

	return ast_compile_expr(env, x, y, n);
}

#undef EPSILON
#undef ANY
#undef NEWSTATE
#undef LITERAL
#undef RECURSE

int
ast_compile(const struct ast *ast,
	struct fsm *fsm, fsm_state_t *start,
	enum re_flags re_flags,
	struct re_err *err)
{
	fsm_state_t x, y;

	assert(ast != NULL);
	assert(start != NULL);

	if (!fsm_addstate(fsm, &x)) {
		return 0;
	}

	if (!fsm_addstate(fsm, &y)) {
		return 0;
	}

	fsm_setend(fsm, y, 1);

	{
		struct comp_env env;

		memset(&env, 0x00, sizeof(env));

		env.fsm = fsm;
		env.re_flags = re_flags;
		env.err = err;

		env.start = x;
		env.end = y;

		if (!ast_compile_root(&env, x, y, ast->expr)) {
			return 0;
		}

		/* env.start may have been modified by fsm_unionxy() during iteration */
		*start = env.start;
	}

/* XXX:
	if (-1 == fsm_trim(fsm)) {
		return 0;
	}
*/

	/*
	 * All flags operators commute with respect to composition.
	 * That is, the order of application here does not matter;
	 * here I'm trying to keep these ordered for efficiency.
	 */

	if (re_flags & RE_REVERSE) {
		if (!fsm_reverse(fsm)) {
			return 0;
		}
	}

	return 1;
}

