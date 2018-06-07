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
	    
	assert(x);
	assert(y);
	if (n == NULL) { return 1; }

	/* fprintf(stderr, "%s: type %d\n", __func__, n->t); */

	switch (n->t) {
	case AST_EXPR_EMPTY:
		if (!fsm_addedge_epsilon(env->fsm, x, y)) { return 0; }
		break;

	case AST_EXPR_LITERAL:
		z = fsm_addstate(env->fsm);
		if (z == NULL) { return 0; }
		if (!addedge_literal(env, x, z, n->u.literal.l.c)) { return 0; }
		if (!comp_iter(env, z, y, n->u.literal.n)) { return 0; }
		break;

	case AST_EXPR_ANY:
		z = fsm_addstate(env->fsm);
		if (z == NULL) { return 0; }
		if (!fsm_addedge_any(env->fsm, x, z)) { return 0; }
		if (!comp_iter(env, z, y, n->u.any.n)) { return 0; }
		break;

	case AST_EXPR_MANY:
		z = fsm_addstate(env->fsm);
		if (z == NULL) { return 0; }
		if (!fsm_addedge_epsilon(env->fsm, x, z)) { return 0; }
		if (!fsm_addedge_any(env->fsm, x, z)) { return 0; }
		if (!fsm_addedge_epsilon(env->fsm, z, x)) { return 0; }
		if (!comp_iter(env, z, y, n->u.any.n)) { return 0; }
		break;

	default:
		assert(0);
	}

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
