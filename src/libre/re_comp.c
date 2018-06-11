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
		NEWSTATE(z);
		RECURSE(x, z, n->u.concat.l);
		RECURSE(z, y, n->u.concat.r);
		break;

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
		if (!addedge_literal(env, x, y, n->u.literal.l.c)) { return 0; }
		break;

	case AST_EXPR_ANY:
		ANY(x, y);
		break;

	case AST_EXPR_MANY:
		NEWSTATE(z);
		EPSILON(x, z);
		ANY(x, z);
		EPSILON(z, x);
		EPSILON(z, y);
		break;

	case AST_EXPR_ATOM:
		switch (n->u.atom.flag) {
		case AST_ATOM_COUNT_FLAG_ONE:
			RECURSE(x, y, n->u.atom.e);
			break;
		case AST_ATOM_COUNT_FLAG_KLEENE:
		case AST_ATOM_COUNT_FLAG_PLUS:
			FAIL("TODO");
			/* fallthrough */
		default:
			FAIL("<matchfail>");
			return 0;
		}
		break;

	default:
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
