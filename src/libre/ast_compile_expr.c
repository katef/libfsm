/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <fsm/fsm.h>

#include <re/re.h>

#include "class.h"
#include "ast.h"

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
	enum re_flags flags;
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
	struct fsm_state *start;
	struct fsm_state *end;
	struct fsm_state *start_any_loop;
	struct fsm_state *end_any_loop;
};

static int
comp_iter(struct comp_env *env,
	struct fsm_state *x, struct fsm_state *y,
	struct ast_expr *n);

static int
addedge_literal(struct comp_env *env,
	struct fsm_state *from, struct fsm_state *to, char c)
{
	struct fsm *fsm = env->fsm;
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);
	
	if (env->flags & RE_ICASE) {
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

static struct fsm_state *
intern_start_any_loop(struct comp_env *env)
{
	struct fsm_state *loop;

	assert(env != NULL);

	if (env->start_any_loop != NULL) {
		return env->start_any_loop;
	}

	assert(~env->flags & RE_ANCHORED);
	assert(env->start != NULL);
		
	loop = fsm_addstate(env->fsm);
	if (loop == NULL) {
		return NULL;
	}

	if (!fsm_addedge_any(env->fsm, loop, loop)) {
		return NULL;
	}

	if (!fsm_addedge_epsilon(env->fsm, env->start, loop)) {
		return NULL;
	}

	env->start_any_loop = loop;

	return env->start_any_loop;
}

static struct fsm_state *
intern_end_any_loop(struct comp_env *env)
{
	struct fsm_state *loop;

	assert(env != NULL);

	if (env->end_any_loop != NULL) {
		return env->end_any_loop;
	}

	assert(~env->flags & RE_ANCHORED);
	assert(env->end != NULL);
		
	loop = fsm_addstate(env->fsm);
	if (loop == NULL) {
		return NULL;
	}

	if (!fsm_addedge_any(env->fsm, loop, loop)) {
		return NULL;
	}

	if (!fsm_addedge_epsilon(env->fsm, loop, env->end)) {
		return NULL;
	}

	env->end_any_loop = loop;

	return env->end_any_loop;
}

static int
can_have_backward_epsilon_edge(const struct ast_expr *e)
{
	switch (e->type) {
	case AST_EXPR_LITERAL:
	case AST_EXPR_FLAGS:
	case AST_EXPR_CLASS:
	case AST_EXPR_ALT_N:
	case AST_EXPR_ANCHOR:
		/* These nodes cannot have a backward epsilon edge */
		return 0;

	case AST_EXPR_REPEATED:
		/* 0 and 1 don't have backward epsilon edges */
		if (e->u.repeated.high <= 1) {
			return 0;
		}

		/*
		 * The general case for counted repetition already
		 * allocates one-way guard states around it
		 */
		if (e->u.repeated.high != AST_COUNT_UNBOUNDED) {
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

	if (r != NULL && r->type == AST_EXPR_REPEATED) {
		if (!can_have_backward_epsilon_edge(r)) {
			return 1;
		}
	}

	return 0;
}

static enum link_types
decide_linking(struct comp_env *env,
	struct fsm_state *x, struct fsm_state *y,
	struct ast_expr *n, enum link_side side)
{
	enum link_types res = LINK_NONE;

	assert(n != NULL);
	assert(env != NULL);

	if ((env->flags & RE_ANCHORED)) {
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

		return LINK_TOP_DOWN;

	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_CLASS:

	case AST_EXPR_CONCAT_N:
	case AST_EXPR_ALT_N:
	case AST_EXPR_REPEATED:
	case AST_EXPR_FLAGS:
	case AST_EXPR_TOMBSTONE:
		break;

	default:
		assert(!"unreached");
	}

	switch (side) {
	case LINK_START: {
		const int start    = (n->type == AST_EXPR_ANCHOR && n->u.anchor.type == AST_ANCHOR_START);
		const int first    = (n->flags & AST_EXPR_FLAG_FIRST) != 0;
		const int nullable = (n->flags & AST_EXPR_FLAG_NULLABLE) != 0;

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
		const int last     = (n->flags & AST_EXPR_FLAG_LAST) != 0;
		const int nullable = (n->flags & AST_EXPR_FLAG_NULLABLE) != 0;

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
    NAME = fsm_addstate(env->fsm);  \
    if (NAME == NULL) { return 0; }

#define EPSILON(FROM, TO)           \
    if (!fsm_addedge_epsilon(env->fsm, FROM, TO)) { return 0; }
        
#define ANY(FROM, TO)               \
    if (!fsm_addedge_any(env->fsm, FROM, TO)) { return 0; }

#define LITERAL(FROM, TO, C)        \
    if (!addedge_literal(env, FROM, TO, C)) { return 0; }

#define RECURSE(FROM, TO, NODE)     \
    if (!comp_iter(env, FROM, TO, NODE)) { return 0; }

static int
comp_iter_repeated(struct comp_env *env,
	struct fsm_state *x, struct fsm_state *y,
	struct ast_expr_repeated *n)
{
	struct fsm_state *a, *b;
	struct fsm_state *na, *nz;
	unsigned i, low, high;

	a = NULL;
	b = NULL;
	
	low  = n->low;
	high = n->high;

	assert(low <= high);

	if (low == 0 && high == 0) {                           /* {0,0} */
		EPSILON(x, y);
	} else if (low == 0 && high == 1) {                    /* '?' */
		RECURSE(x, y, n->e);
		EPSILON(x, y);
	} else if (low == 1 && high == 1) {                    /* {1,1} */
		RECURSE(x, y, n->e);
	} else if (low == 0 && high == AST_COUNT_UNBOUNDED) {  /* '*' */
		EPSILON(x, y);
		RECURSE(x, y, n->e);
		EPSILON(y, x);
	} else if (low == 1 && high == AST_COUNT_UNBOUNDED) {  /* '+' */
		RECURSE(x, y, n->e);
		EPSILON(y, x);
	} else {
		/*
		 * Make new beginning/end states for the repeated section,
		 * build its NFA, and link to its head.
		 */

		NEWSTATE(na);
		NEWSTATE(nz);
		RECURSE(na, nz, n->e);
		EPSILON(x, na); /* link head to repeated NFA head */

		b = nz; /* set the initial tail */

		/* can be skipped */
		if (low == 0) {
			EPSILON(na, nz);
		}
		
		for (i = 1; i < high; i++) {
			a = fsm_state_duplicatesubgraphx(env->fsm, na, &b);
			if (a == NULL) {
				return 0;
			}
			
			/*
			 * TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state
			 */
			EPSILON(nz, a);
			
			/* To the optional part of the repeated count */
			if (i >= low) {
				EPSILON(nz, b);
			}
			
			na = a;	/* advance head for next duplication */
			nz = b;	/* advance tail for concenation */
		}
		
		/* tail to last repeated NFA tail */
		EPSILON(nz, y);
	}
	return 1;
}

static int
comp_iter(struct comp_env *env,
	struct fsm_state *x, struct fsm_state *y,
	struct ast_expr *n)
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
			x = intern_start_any_loop(env);
			assert(env->start_any_loop != NULL);
			if (x == NULL) { return 0; }
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
			struct fsm_state *start_any_loop = intern_start_any_loop(env);
			if (start_any_loop == NULL) { return 0; }
			assert((link_start & LINK_GLOBAL) == LINK_NONE);
			EPSILON(start_any_loop, x);
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
			y = intern_end_any_loop(env);
			if (y == NULL) { return 0; }
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
			struct fsm_state *end_any_loop = intern_end_any_loop(env);
			if (end_any_loop == NULL) { return 0; }
			assert((link_end & LINK_GLOBAL) == LINK_NONE);
			EPSILON(y, end_any_loop);
		}
	}

	assert(x != NULL);
	assert(y != NULL);

	switch (n->type) {
	case AST_EXPR_EMPTY:
		/* skip these, when possible */
		EPSILON(x, y);
		break;

	case AST_EXPR_CONCAT_N:
	{
		struct fsm_state *z, *right;
		struct fsm_state *curr_x;
		enum re_flags saved;
		size_t i;

		const size_t count  = n->u.concat_n.count;

		curr_x = x;
		saved  = env->flags;

		assert(count >= 1);

		NEWSTATE(z);

		for (i = 0; i < count; i++) {
			struct ast_expr *curr = n->u.concat_n.n[i];
			struct ast_expr *next = i == count - 1
				? NULL
				: n->u.concat_n.n[i + 1];

			if (curr->type == AST_EXPR_FLAGS) {
				/*
				 * Save the current flags in the flags node,
				 * restore when done evaluating the concat
				 * node's right subtree.
				 */
				saved = env->flags;
				
				/*
				 * Note: in cases like `(?i-i)`, the negative is
				 * required to take precedence.
				 */
				env->flags |=  curr->u.flags.pos;
				env->flags &= ~curr->u.flags.neg;
			}

			/*
			 * If nullable, add an extra state & epsilion as a one-way gate
			 */
			if (!can_skip_concat_state_and_epsilon(curr, next)) {
				struct fsm_state *diode;

				NEWSTATE(diode);
				EPSILON(curr_x, diode);
				curr_x = diode;
			}

			right = (i < count - 1 ? z : y);
			RECURSE(curr_x, right, curr);

			if (i < count - 1) {
				struct fsm_state *zn;
				curr_x = z;
				NEWSTATE(zn);
				z = zn;
			}
		}

		env->flags = saved;

		break;
	}

	case AST_EXPR_ALT_N:
	{
		size_t i;

		const size_t count = n->u.alt_n.count;

		assert(count > 1);

		for (i = 0; i < count; i++) {
			/*
			 * CONCAT handles adding extra states and
			 * epsilons when necessary, so there isn't much
			 * more to do here.
			 */
			RECURSE(x, y, n->u.alt_n.n[i]);
		}		
		break;
	}

	case AST_EXPR_LITERAL:
		LITERAL(x, y, n->u.literal.c);
		break;

	case AST_EXPR_ANY:
		ANY(x, y);
		break;

	case AST_EXPR_REPEATED:
		/*
		 * REPEATED breaks out into its own function, because
		 * there are several special cases
		 */
		if (!comp_iter_repeated(env, x, y, &n->u.repeated)) {
			return 0;
		}
		break;

	case AST_EXPR_CLASS:
		/*
		 * XXX: this doesn't belong here; it's set as a fall-through.
		 * Instead we should be populating .start/.end for each node
		 * in the char class tree.
		 */
		if (env->err != NULL) {
			env->err->start.byte = n->u.class.start.byte;
			env->err->end.byte   = n->u.class.end.byte;
		}

		if (!ast_compile_class(n->u.class.class,
			env->fsm, env->flags, env->err, x, y)) {
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

int
ast_compile_expr(struct ast_expr *n,
	struct fsm *fsm, enum re_flags flags,
	struct re_err *err,
	struct fsm_state *x, struct fsm_state *y)
{
	struct comp_env env;

	assert(n != NULL);
	assert(fsm != NULL);
	assert(x != NULL);
	assert(y != NULL);

	memset(&env, 0x00, sizeof(env));

	env.fsm = fsm;
	env.flags = flags;
	env.err = err;

	env.start = x;
	env.end = y;

	if (!comp_iter(&env, x, y, n)) {
		return 0;
	}

	if (-1 == fsm_trim(env.fsm)) {
		return 0;
	}

	return 1;
}

