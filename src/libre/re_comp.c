/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "re_comp.h"

#include <ctype.h>

struct comp_env {
	struct fsm *fsm;
	enum re_flags flags;
	const struct fsm_options *opt;
	struct re_err *err;
};

static int
addedge_literal(struct comp_env *env,
    struct fsm_state *from, struct fsm_state *to, char c);

static int
comp_iter_repeated(struct comp_env *env,
    struct fsm_state *x, struct fsm_state *y,
    struct ast_expr_repeated *n);


#define NEWSTATE(NAME)						       \
	NAME = fsm_addstate(env->fsm);				       \
	if (NAME == NULL) { return 0; }				       \

#define EPSILON(FROM, TO)					       \
	if (!fsm_addedge_epsilon(env->fsm, FROM, TO)) { return 0; }
	    
#define ANY(FROM, TO)						       \
	if (!fsm_addedge_any(env->fsm, FROM, TO)) { return 0; }

#define RECURSE(FROM, TO, NODE)					       \
	if (!comp_iter(env, FROM, TO, NODE)) { return 0; }             \

static int
can_skip_concat_state_and_epsilon(const struct ast_expr *l,
    const struct ast_expr *r);

static int
comp_iter(struct comp_env *env,
    struct fsm_state *x, struct fsm_state *y,
    struct ast_expr *n)
{
	struct fsm_state *z;

	assert(x);
	assert(y);
	if (n == NULL) { return 1; }

	switch (n->t) {
	case AST_EXPR_EMPTY:
		EPSILON(x, y);
		break;

	case AST_EXPR_CONCAT:
	{
		struct fsm_state *z2;
		struct ast_expr *l, *r;
		NEWSTATE(z);
		assert(n->u.concat.l != NULL);
		assert(n->u.concat.r != NULL);
		l = n->u.concat.l;
		r = n->u.concat.r;
		if (l->t == AST_EXPR_FLAGS) {
			/* Save the current flags in the flags node,
			 * restore when done evaluating the concat
			 * node's right subtree. */
			l->u.flags.saved = env->flags;

			/* Note: in cases like `(?i-i)`, the negative is
			 * required to take precedence. */
			env->flags |= l->u.flags.pos;
			env->flags &=~ l->u.flags.neg;
		}

		/* Check if we can safely skip adding a state and epsilon edge */
		if (can_skip_concat_state_and_epsilon(l, r)) {
			RECURSE(x, z, l);
			RECURSE(z, y, r);
		} else {
			NEWSTATE(z2);
			RECURSE(x, z, l);
			EPSILON(z, z2);
			RECURSE(z2, y, r);
		}

		if (l->t == AST_EXPR_FLAGS) {
			env->flags = l->u.flags.saved;
		}
		break;
	}

	case AST_EXPR_ALT:
	{
		struct fsm_state *la, *lz, *ra, *rz;
		struct ast_expr *l, *r;
		assert(n->u.alt.l != NULL);
		assert(n->u.alt.r != NULL);

		l = n->u.alt.l;
		r = n->u.alt.r;

		/* Optimization: for (x|y) with two literal characters,
		 * we don't need to add four states here. */
		if (l->t == AST_EXPR_LITERAL && r->t == AST_EXPR_LITERAL) {
			RECURSE(x, y, l);
			RECURSE(x, y, r);
		} else {
			NEWSTATE(la);
			NEWSTATE(lz);
			NEWSTATE(ra);
			NEWSTATE(rz);
			
			EPSILON(x, la);
			EPSILON(x, ra);
			EPSILON(lz, y);
			EPSILON(rz, y);
			
			RECURSE(la, lz, l);
			RECURSE(ra, rz, r);
		}
		break;
	}

	case AST_EXPR_LITERAL:
		if (!addedge_literal(env, x, y, n->u.literal.c)) { return 0; }
		break;

	case AST_EXPR_ANY:
		ANY(x, y);
		break;

	case AST_EXPR_REPEATED:
		/* REPEATED breaks out into its own function, because
		 * there are several special cases */
		if (!comp_iter_repeated(env, x, y, &n->u.repeated)) { return 0; }
		break;

	case AST_EXPR_CHAR_CLASS:
		if (!re_char_class_ast_compile(n->u.char_class.cca,
			env->fsm, env->flags, env->err, env->opt, x, y)) {
			return 0;
		}
		break;

#if 0
	case AST_EXPR_CHAR_TYPE:
	{
		int i;
		struct re_char_class *cc = re_char_class_type_compile(n->u.char_type.id);
		if (cc == NULL) { return 0; }

		for (i = 0; i < 256; i++) {
			if (cc->chars[i/8] & (1U << (i & 0x07))) {
				if (!addedge_literal(env, x, y, i)) { return 0; }
			}
		}
		re_char_class_free(cc);
		break;
	}
#endif

	case AST_EXPR_GROUP:
		RECURSE(x, y, n->u.group.e);
		break;

	case AST_EXPR_FLAGS:
		/* This is purely a metadata node, handled at analysis
		 * time; just bridge the start and end nodes. */
		EPSILON(x, y);
		break;

	default:
		fprintf(stderr, "%s:%d: <matchfail %d>\n",
		    __FILE__, __LINE__, n->t);
		abort();
	}
	return 1;
}

static int
comp_iter_repeated(struct comp_env *env,
    struct fsm_state *x, struct fsm_state *y,
    struct ast_expr_repeated *n)
{
	struct fsm_state *na, *nz;
	unsigned i, low, high;
	struct fsm_state *a = NULL;
	struct fsm_state *b = NULL;
	
	low = n->low;
	high = n->high;
	assert(low <= high);

	if (low == 0 && high == 0) {		                /* {0,0} */
		EPSILON(x, y);
	} else if (low == 0 && high == 1) {		        /* '?' */
		RECURSE(x, y, n->e);
		EPSILON(x, y);
	} else if (low == 1 && high == 1) {		        /* {1,1} */
		RECURSE(x, y, n->e);
	} else if (low == 0 && high == AST_COUNT_UNBOUNDED) {   /* '*' */
		EPSILON(x, y);
		RECURSE(x, y, n->e);
		EPSILON(y, x);
	} else if (low == 1 && high == AST_COUNT_UNBOUNDED) {   /* '+' */
		RECURSE(x, y, n->e);
		EPSILON(y, x);
	} else {
		/* Make new beginning/end states for the repeated section,
		 * build its NFA, and link to its head. */
		NEWSTATE(na);
		NEWSTATE(nz);
		RECURSE(na, nz, n->e);
		EPSILON(x, na); /* link head to repeated NFA head */
		b = nz;		/* set the initial tail */
		
		if (low == 0) { EPSILON(na, nz); } /* can be skipped */
		
		for (i = 1; i < high; i++) {
			a = fsm_state_duplicatesubgraphx(env->fsm, na, &b);
			if (a == NULL) { return 0; }
			
			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			EPSILON(nz, a);
			
			/* To the optional part of the repeated count */
			if (i >= low) { EPSILON(nz, b); }
			
			na = a;	/* advance head for next duplication */
			nz = b;	/* advance tail for concenation */
		}
		
		EPSILON(nz, y);	     /* tail to last repeated NFA tail */
	}
	return 1;
}

#undef EPSILON
#undef ANY
#undef NEWSTATE

static int
can_have_backward_epsilon_edge(const struct ast_expr *e)
{
	/* These nodes cannot have a backward epsilon edge */
	if (e->t == AST_EXPR_LITERAL) { return 0; }
	if (e->t == AST_EXPR_FLAGS) { return 0; }
	if (e->t == AST_EXPR_CHAR_CLASS) { return 0; }
	if (e->t == AST_EXPR_ALT) { return 0; }

	if (e->t == AST_EXPR_REPEATED) {
		/* 0 and 1 don't have backward epsilon edges */
		if (e->u.repeated.high <= 1) { return 0; }

		/* The general case for counted repetition already
		 * allocates one-way guard states around it */
		if (e->u.repeated.high != AST_COUNT_UNBOUNDED) { return 0; }
	} else if (e->t == AST_EXPR_GROUP) {
		return can_have_backward_epsilon_edge(e->u.group.e);
	}

	return 1;
}

static int
can_skip_concat_state_and_epsilon(const struct ast_expr *l,
    const struct ast_expr *r)
{
	/* CONCAT only needs the extra state and epsilon edge when there
	 * is a backward epsilon node for repetition -- without it, a
	 * regex such as /a*b*c/ could match "ababc" as well as "aabbc",
	 * because the backward epsilon for repeating the 'b' would lead
	 * to a state which has another backward epsilon for repeating
	 * the 'a'. The extra state functions as a one-way guard,
	 * keeping the match from looping further back in the FSM than
	 * intended. */

	if (!can_have_backward_epsilon_edge(l)) { return 1; }

	if (r->t == AST_EXPR_REPEATED) {
		if (!can_have_backward_epsilon_edge(r)) { return 1; }
	}

	return 0;
}

static struct fsm *
new_blank(const struct fsm_options *opt)
{
	struct fsm *new;
	struct fsm_state *start;
	
	new = fsm_new(opt);
	if (new == NULL) {
		return NULL;
	}
	
	start = fsm_addstate(new);
	if (start == NULL) {
		goto error;
	}
	
	fsm_setstart(new, start);
	
	return new;
	
error:
	
	fsm_free(new);
	
	return NULL;
}

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


struct fsm *
re_comp_ast(struct ast_re *ast,
    enum re_flags flags,
    const struct fsm_options *opt)
{
	struct fsm_state *x, *y;
	struct comp_env env;
	assert(ast);

	env.fsm = new_blank(opt);
	if (env.fsm == NULL) {
		return NULL;
	}
	env.opt = opt;
	env.flags = flags;
	
	x = fsm_getstart(env.fsm);
	assert(x != NULL);
	
	y = fsm_addstate(env.fsm);
	if (y == NULL) {
		goto error;
	}
	
	fsm_setend(env.fsm, y, 1);

	if (!comp_iter(&env, x, y, ast->expr)) { goto error; }

	return env.fsm;

error:
	fsm_free(env.fsm);
	return NULL;
}
