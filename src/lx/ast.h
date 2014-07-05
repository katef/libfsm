/* $Id$ */

#ifndef LX_AST_H
#define LX_AST_H


#include <re/re.h>


struct re;

struct ast_mapping {
	struct re *re;
	struct ast_token *token;
	struct ast_zone  *to;

	struct ast_mapping *next;
};

struct ast_zone {
	struct ast_mapping *ml;
	struct re *re;

	struct ast_zone *next;
};

struct ast_token {
	const char *s;

	struct ast_token *next;
};

struct ast {
	struct ast_zone  *zl;
	struct ast_token *tl;

	struct ast_zone *global;
};


struct ast *
ast_new(void);

struct ast_token *
ast_addtoken(struct ast *ast, const char *s);

struct ast_zone *
ast_addzone(struct ast *ast);

struct ast_mapping *
ast_addmapping(struct ast_zone *z, struct re *re,
	struct ast_token *token, struct ast_zone *to);


#endif

