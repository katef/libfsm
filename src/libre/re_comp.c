#include "re_comp.h"

#include <ctype.h>

struct comp_env {
	struct fsm *fsm;
	enum re_flags flags;
	const struct fsm_options *opt;
};

static int
addedge_literal(struct comp_env *env,
    struct fsm_state *from, struct fsm_state *to, char c);

static int
comp_iter(struct comp_env *env,
    struct fsm_state *x, struct fsm_state *y,
    struct ast_expr *n)
{
	struct fsm_state *z;

#define NEWSTATE(NAME)						       \
	NAME = fsm_addstate(env->fsm);				       \
	if (NAME == NULL) { return 0; }				       \

#define EPSILON(FROM, TO)					       \
	if (!fsm_addedge_epsilon(env->fsm, FROM, TO)) { return 0; }
	    
#define ANY(FROM, TO)						       \
	if (!fsm_addedge_any(env->fsm, FROM, TO)) { return 0; }

#define RECURSE(FROM, TO, NODE)					       \
	if (!comp_iter(env, FROM, TO, NODE)) { return 0; }             \

	assert(x);
	assert(y);
	if (n == NULL) { return 1; }

	/* fprintf(stderr, "%s: type %d\n", __func__, n->t); */

	switch (n->t) {
	case AST_EXPR_EMPTY:
		EPSILON(x, y);
		break;

	case AST_EXPR_CONCAT:
	{
		struct fsm_state *z2;
		NEWSTATE(z);
		assert(n->u.concat.l != NULL);
		assert(n->u.concat.r != NULL);
		if (n->u.concat.l->t == AST_EXPR_LITERAL) {
			RECURSE(x, z, n->u.concat.l);
			RECURSE(z, y, n->u.concat.r);
		} else {
			NEWSTATE(z2);
			RECURSE(x, z, n->u.concat.l);
			EPSILON(z, z2);
			RECURSE(z2, y, n->u.concat.r);
		}
		break;
	}

	case AST_EXPR_ALT:
	{
		struct fsm_state *la, *lz, *ra, *rz;
		NEWSTATE(la);
		NEWSTATE(lz);
		NEWSTATE(ra);
		NEWSTATE(rz);

		EPSILON(x, la);
		EPSILON(x, ra);
		EPSILON(lz, y);
		EPSILON(rz, y);

		RECURSE(la, lz, n->u.alt.l);
		RECURSE(ra, rz, n->u.alt.r);
		break;
	}

	case AST_EXPR_LITERAL:
		if (!addedge_literal(env, x, y, n->u.literal.c)) { return 0; }
		break;

	case AST_EXPR_ANY:
		ANY(x, y);
		break;

	case AST_EXPR_MANY:
		/* FIXME: is z necessary? */
		NEWSTATE(z);
		EPSILON(x, z);
		ANY(x, z);
		EPSILON(z, x);
		EPSILON(z, y);
		break;

	case AST_EXPR_KLEENE:
		EPSILON(x, y);
		RECURSE(x, y, n->u.alt.l);
		EPSILON(y, x);
		break;

	case AST_EXPR_PLUS:
		RECURSE(x, y, n->u.alt.l);
		EPSILON(y, x);
		break;

	case AST_EXPR_OPT:
		RECURSE(x, y, n->u.alt.l);
		EPSILON(x, y);
		break;

	case AST_EXPR_REPEATED:
	{
		struct fsm_state *na, *nz;
		unsigned i, low, high;
		struct fsm_state *a = NULL;
		struct fsm_state *b = NULL;

		/* This should have been normalized to [M;N], where
		 * M < N and both are bounded, and {0,1} is built as
		 * AST_EXPR_OPT instead. */
		low = n->u.repeated.low;
		high = n->u.repeated.high;
		assert(low <= high);
		assert(low != AST_COUNT_UNBOUNDED);
		assert(high != AST_COUNT_UNBOUNDED);

		/* Make new beginning/end states for the repeated section,
		 * build its NFA, and link to its head. */
		NEWSTATE(na);
		NEWSTATE(nz);
		RECURSE(na, nz, n->u.repeated.e);
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
		break;
	}

	case AST_EXPR_CLASS:
	{
		int i;
		struct re_char_class_ast *cca = n->u.class.cca;
		struct re_char_class *cc = re_char_class_ast_compile(cca);
		if (cc == NULL) { return 0; }

		for (i = 0; i < 256; i++) {
			if (cc->chars[i/8] & (1U << (i & 0x07))) {
				if (!addedge_literal(env, x, y, i)) { return 0; }
			}
		}

		break;
	}

	case AST_EXPR_GROUP:
		/* TODO: annotate the FSM with group match info */
		RECURSE(x, y, n->u.group.e);
		break;

	default:
		fprintf(stderr, "%s:%d: <matchfail %d>\n",
		    __FILE__, __LINE__, n->t);
		abort();
	}

#undef EPSILON
#undef ANY
#undef NEWSTATE
	return 1;
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
