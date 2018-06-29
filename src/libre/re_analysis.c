/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "re_analysis.h"

#include <string.h>


struct analysis_env {
	unsigned group_id;
};

static void
analysis_iter(struct analysis_env *env, struct ast_expr *n);

static void
analysis_char_class(struct analysis_env *env, struct re_char_class_ast *cca);	

void
re_ast_analysis(struct ast_re *ast)
{
	struct analysis_env env;
	if (ast == NULL) { return; }
	memset(&env, 0x00, sizeof(env));

	assert(ast->expr != NULL);
	analysis_iter(&env, ast->expr);
}

/* Now that the AST is complete, do a pass filling in details. */
/* TODO: extract traversal from the callback, so the individual
 * analysis passes have an obvious focus. */
static void
analysis_iter(struct analysis_env *env, struct ast_expr *n)
{
	switch (n->t) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
		break;
	case AST_EXPR_CONCAT:
		analysis_iter(env, n->u.concat.l);
		analysis_iter(env, n->u.concat.r);
		break;
	case AST_EXPR_ALT:
		analysis_iter(env, n->u.alt.l);
		analysis_iter(env, n->u.alt.r);
		break;

	case AST_EXPR_ANY:
	case AST_EXPR_MANY:
	case AST_EXPR_FLAGS:
		break;

	case AST_EXPR_REPEATED:
		analysis_iter(env, n->u.repeated.e);
		break;
	case AST_EXPR_CHAR_CLASS:
		analysis_char_class(env, n->u.char_class.cca);
		break;
	case AST_EXPR_GROUP:
		/* assign group ID */
		env->group_id++;
		n->u.group.id = env->group_id;
		analysis_iter(env, n->u.group.e);
		break;

	default:
		fprintf(stderr, "%s:%d: <matchfail %d>\n",
		    __FILE__, __LINE__, n->t);
		abort();
	}
}

static void
analysis_char_class(struct analysis_env *env, struct re_char_class_ast *cca)
{
	(void)env;
	(void)cca;
}

