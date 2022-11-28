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
#include <fsm/subgraph.h>

#include <re/re.h>

#include "class.h"
#include "ast.h"
#include "ast_compile.h"

#include "libfsm/internal.h" /* XXX */

#define LOG_LINKAGE 0

#if LOG_LINKAGE
#include "print.h"
#endif

enum link_side {
	LINK_START,
	LINK_END
};

/*
 * How should this state be linked for the relevant state?
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
	LINK_TOP_DOWN,
	LINK_GLOBAL,
	LINK_GLOBAL_SELF_LOOP,
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
	fsm_state_t end_nl;
	int has_start_any_loop;
	int has_end_any_loop;
	int has_end_nl;
};

static int
comp_iter(struct comp_env *env,
	fsm_state_t x, fsm_state_t y,
	struct ast_expr *n, const struct ast_expr *parent);

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
static int
fsm_unionxy(struct fsm *a, struct fsm *b, fsm_state_t x, fsm_state_t y)
{
	fsm_state_t sa, sb;
	fsm_state_t end;
	struct fsm *q;
	fsm_state_t base_b;

	assert(a != NULL);
	assert(b != NULL);

	/* x,y both belong to a */
	assert(x < a->statecount);
	assert(y < a->statecount);

	if (!fsm_getstart(a, &sa)) {
		return 0;
	}

	if (!fsm_getstart(b, &sb)) {
		return 0;
	}

	if (!fsm_collate(b, &end, fsm_isend)) {
		return 0;
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
		return 0;
	}

	sb  += base_b;
	end += base_b;

	fsm_setstart(q, sa);

	if (!fsm_addedge_epsilon(q, x, sb)) {
		return 0;
	}

	if (!fsm_addedge_epsilon(q, end, y)) {
		return 0;
	}

	return 1;
}

static struct fsm *
expr_compile(struct ast_expr *e, enum re_flags flags,
	const struct fsm_options *opt, struct re_err *err)
{
	struct ast ast;

	ast.expr = e;

	return ast_compile(&ast, flags, opt, err);
}

static int
addedge_literal(struct comp_env *env, enum re_flags re_flags,
	fsm_state_t from, fsm_state_t to, char c)
{
	struct fsm *fsm = env->fsm;
	assert(fsm != NULL);

	assert(from < env->fsm->statecount);
	assert(to < env->fsm->statecount);

	if (re_flags & RE_ICASE) {
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

	if (env->has_start_any_loop) {
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
	env->has_start_any_loop = 1;

	return 1;
}

static int
intern_end_any_loop(struct comp_env *env)
{
	fsm_state_t loop;

	assert(env != NULL);

	if (env->has_end_any_loop) {
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
	env->has_end_any_loop = 1;

	return 1;
}

static int
intern_end_nl(struct comp_env *env)
{
	/* PCRE's end anchor $ matches a single optional newline.
	 *
	 * Intern states for a `\n?` that links to the global end. */
	assert(env != NULL);

	if (env->has_end_nl) {
		return 1;
	}

	assert(~env->re_flags & RE_ANCHORED);
	assert(env->re_flags & RE_END_NL);
	assert(env->end < env->fsm->statecount);

	fsm_state_t end_nl;
	if (!fsm_addstate(env->fsm, &end_nl)) {
		return 0;
	}

#if LOG_LINKAGE
	fprintf(stderr, "%s: end_nl: %d\n", __func__, end_nl);
#endif

	if (!fsm_addedge_epsilon(env->fsm, end_nl, env->end)) {
		return 0;
	}
	if (!fsm_addedge_literal(env->fsm, end_nl, env->end, (char)'\n')) {
		return 0;
	}

	env->end_nl = end_nl;
	env->has_end_nl = 1;
	return 1;
}

static int
can_have_backward_epsilon_edge(const struct ast_expr *e)
{
	switch (e->type) {
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
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
	struct ast_expr *n, const struct ast_expr *parent, enum link_side side)
{
	assert(n != NULL);
	assert(env != NULL);

	/* If the regex is implicitly anchored and the dialect does
	 * not support anchoring, linking is always top-down. */
	if ((env->re_flags & RE_ANCHORED)) {
		return LINK_TOP_DOWN;
	}

	/* parent can be NULL, if we're at the root node, but it must
	 * never be the same node. */
	assert(parent != n);

	/* Note: any asymmetry here should be due to special cases
	 * involving `$` matching exactly one '\n'. */
	switch (side) {
	case LINK_START: {
		const int first    = (n->flags & AST_FLAG_FIRST) != 0;
		const int anchored = (n->flags & AST_FLAG_ANCHORED_START) != 0;
		const int anchored_parent = (parent && (parent->flags & AST_FLAG_ANCHORED_START) != 0);

		if (first && anchored) { return LINK_GLOBAL; }
		if (first && !anchored) { return LINK_GLOBAL_SELF_LOOP; }

		if (anchored && !first && !anchored_parent) {
			return LINK_GLOBAL;
		}

		return LINK_TOP_DOWN;

	}

	case LINK_END: {
		const int last     = (n->flags & AST_FLAG_LAST) != 0;
		const int anchored = (n->flags & AST_FLAG_ANCHORED_END) != 0;

		if (last && anchored) { return LINK_GLOBAL; }
		if (last && !anchored) { return LINK_GLOBAL_SELF_LOOP; }

		if (anchored && !last) {
			return LINK_GLOBAL;
		}

		return LINK_TOP_DOWN;
	}

	default:
		assert(!"unreached");
	}

	return LINK_GLOBAL;
}

static void
print_linkage(enum link_types t)
{
	switch (t) {
	case LINK_TOP_DOWN:
		fprintf(stderr, "[TOP_DOWN]");
		break;
	case LINK_GLOBAL:
		fprintf(stderr, "[GLOBAL]");
		break;
	case LINK_GLOBAL_SELF_LOOP:
		fprintf(stderr, "[SELF_LOOP]");
		break;
	default:
		assert(!"match fail");
		break;
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
    if (!addedge_literal(env, n->re_flags, (FROM), (TO), ((char)C))) { return 0; }

#define RECURSE(FROM, TO, NODE, PARENT)		\
    if (!comp_iter(env, (FROM), (TO), (NODE), (PARENT))) { return 0; }

static int
comp_iter_repeated(struct comp_env *env,
	fsm_state_t x, fsm_state_t y,
	struct ast_expr *n)
{
	fsm_state_t a, b;
	fsm_state_t na, nz;
	unsigned i;

	assert(n->type == AST_EXPR_REPEAT);
	const unsigned min = n->u.repeat.min;
	const unsigned max = n->u.repeat.max;
	struct ast_expr *e = n->u.repeat.e;

	assert(min <= max);

	if (min == 0 && max == 0) {                          /* {0,0} */
		EPSILON(x, y);
	} else if (min == 0 && max == 1) {                   /* '?' */
		RECURSE(x, y, e, n);
		EPSILON(x, y);
	} else if (min == 1 && max == 1) {                   /* {1,1} */
		RECURSE(x, y, e, n);
	} else if (min == 0 && max == AST_COUNT_UNBOUNDED) { /* '*' */
		NEWSTATE(na);
		NEWSTATE(nz);
		EPSILON(x,na);
		EPSILON(nz,y);

		EPSILON(na, nz);
		RECURSE(na, nz, e, n);
		EPSILON(nz, na);
	} else if (min == 1 && max == AST_COUNT_UNBOUNDED) { /* '+' */
		NEWSTATE(na);
		NEWSTATE(nz);
		EPSILON(x,na);
		EPSILON(nz,y);

		RECURSE(na, nz, e, n);
		EPSILON(nz, na);
	} else {
		/*
		 * Make new beginning/end states for the repeated section,
		 * build its NFA, and link to its head.
		 */

		struct fsm_subgraph subgraph;
		fsm_state_t tail;

		fsm_subgraph_start(env->fsm, &subgraph);

		NEWSTATE(na);
		NEWSTATE(nz);
		RECURSE(na, nz, e, n);
		EPSILON(x, na); /* link head to repeated NFA head */

		b = nz; /* set the initial tail */

		/* can be skipped */
		if (min == 0) {
			EPSILON(na, nz);
		}
		fsm_subgraph_stop(env->fsm, &subgraph);
		tail = nz;

		if (max != AST_COUNT_UNBOUNDED) {
			for (i = 1; i < max; i++) {
				/* copies the original subgraph; need to set b to the
				 * original tail
				 */
				b = tail;

				if (!fsm_subgraph_duplicate(env->fsm, &subgraph, &b, &a)) {
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

				if (!fsm_subgraph_duplicate(env->fsm, &subgraph, &b, &a)) {
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
comp_iter(struct comp_env *env,
	fsm_state_t x, fsm_state_t y,
	struct ast_expr *n, const struct ast_expr *parent)
{
	enum link_types link_start, link_end;

	if (n == NULL) {
		return 1;
	}

	link_start = decide_linking(env, n, parent, LINK_START);
	link_end   = decide_linking(env, n, parent, LINK_END);

#if LOG_LINKAGE
	fprintf(stderr, "%s: decide_linking %p: start ", __func__, (void *) n);
	print_linkage(link_start);
	fprintf(stderr, ", end ");
	print_linkage(link_end);
	fprintf(stderr, ", x %d, y %d\n", x, y);
#else
	(void) print_linkage;
#endif

	switch (link_start) {
	case LINK_TOP_DOWN:
		break;
	case LINK_GLOBAL:
		x = env->start;
		break;
	case LINK_GLOBAL_SELF_LOOP:
		if (!intern_start_any_loop(env)) {
			return 0;
		}
		assert(env->has_start_any_loop);

		x = env->start_any_loop;
		break;
	default:
		assert(!"unreachable");
	}

	switch (link_end) {
	case LINK_TOP_DOWN:
		break;
	case LINK_GLOBAL:
		if (env->re_flags & RE_END_NL && (n->flags & AST_FLAG_END_NL)) {
			if (!intern_end_nl(env)) {
				return 0;
			}
			y = env->end_nl;
		} else {
			y = env->end;
		}
		break;
	case LINK_GLOBAL_SELF_LOOP:
		if (!intern_end_any_loop(env)) {
			return 0;
		}
		assert(env->has_end_any_loop);

		y = env->end_any_loop;
		break;
	default:
		assert(!"unreachable");
	}

#if LOG_LINKAGE
	fprintf(stderr, " ---> x: %d, y: %d\n", x, y);
#endif

	switch (n->type) {
	case AST_EXPR_EMPTY:
		/* skip these, when possible */
		EPSILON(x, y);
		break;

	case AST_EXPR_CONCAT:
	{
		fsm_state_t curr_x;
		size_t i;

		const size_t count  = n->u.concat.count;

		curr_x = x;

		assert(count >= 1);

		if (!fsm_addstate_bulk(env->fsm, count - 1)) {
			return 0;
		}

		for (i = 0; i < count; i++) {
			struct ast_expr *curr = n->u.concat.n[i];

			/* If a subtree is unsatisfiable but also nullable, ignore it. */
			const enum ast_flags nullable_and_unsat = AST_FLAG_NULLABLE
			    | AST_FLAG_UNSATISFIABLE;
			if ((curr->flags & nullable_and_unsat) == nullable_and_unsat) {
				/* if necessary, link the end */
				if (i == count - 1) {
					EPSILON(curr_x, y);
				}
				continue;
			}

			struct ast_expr *next = i == count - 1
				? NULL
				: n->u.concat.n[i + 1];

			fsm_state_t z;
			if (i + 1 < count) {
				if (!fsm_addstate(env->fsm, &z)) {
					return 0;
				}
#if LOG_LINKAGE
				fprintf(stderr, "%s: added state z %d\n", __func__, z);
#endif
			} else {
				z = y; /* connect to right parent to close off subtree */
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

			RECURSE(curr_x, z, curr, n);

			curr_x = z;
		}

		break;
	}

	case AST_EXPR_ALT:
	{
		size_t i;

		const size_t count = n->u.alt.count;

		assert(count >= 1);

		for (i = 0; i < count; i++) {
			/* skip unsatisfiable ALT subtrees */
			if (n->u.alt.n[i]->flags & AST_FLAG_UNSATISFIABLE) {
				continue;
			}

			/*
			 * CONCAT handles adding extra states and
			 * epsilons when necessary, so there isn't much
			 * more to do here.
			 */
			RECURSE(x, y, n->u.alt.n[i], n);
		}
		break;
	}

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

	case AST_EXPR_REPEAT:
		/*
		 * REPEAT breaks out into its own function, because
		 * there are several special cases
		 */
		if (!comp_iter_repeated(env, x, y, n)) {
			return 0;
		}
		break;

	case AST_EXPR_GROUP:
		RECURSE(x, y, n->u.group.e, n);
		break;

	case AST_EXPR_TOMBSTONE:
		/* do not link -- intentionally pruned */
		break;

	case AST_EXPR_ANCHOR:
		EPSILON(x, y);
		break;

	case AST_EXPR_SUBTRACT: {
		struct fsm *a, *b;
		struct fsm *q;
		enum re_flags re_flags;

		re_flags = n->re_flags;

		/* wouldn't want to reverse twice! */
		re_flags &= ~(unsigned)RE_REVERSE;

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

		if (!fsm_unionxy(env->fsm, q, x, y)) {
			return 0;
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

		if (n->u.range.from.u.literal.c == 0x00 &&
			n->u.range.to.u.literal.c == 0xff)
		{
			ANY(x, y);
			break;
		}

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

#undef EPSILON
#undef ANY
#undef NEWSTATE
#undef LITERAL
#undef RECURSE

struct fsm *
ast_compile(const struct ast *ast,
	enum re_flags re_flags,
	const struct fsm_options *opt,
	struct re_err *err)
{
	fsm_state_t x, y;
	struct fsm *fsm;

	assert(ast != NULL);

	fsm = fsm_new(opt);
	if (fsm == NULL) {
		return NULL;
	}

	if (!fsm_addstate(fsm, &x)) {
		goto error;
	}

	if (!fsm_addstate(fsm, &y)) {
		goto error;
	}

	fsm_setstart(fsm, x);
	fsm_setend(fsm, y, 1);

	{
		struct comp_env env;

		memset(&env, 0x00, sizeof(env));

		env.fsm = fsm;
		env.re_flags = re_flags;
		env.err = err;

		env.start = x;
		env.end = y;

		if (!comp_iter(&env, x, y, ast->expr, NULL)) {
			goto error;
		}
	}

/* XXX:
	if (-1 == fsm_trim(fsm)) {
		goto error;
	}
*/

	/*
	 * All flags operators commute with respect to composition.
	 * That is, the order of application here does not matter;
	 * here I'm trying to keep these ordered for efficiency.
	 */

	if (re_flags & RE_REVERSE) {
		if (!fsm_reverse(fsm)) {
			goto error;
		}
	}

	return fsm;

error:

	fsm_free(fsm);

	return NULL;
}

