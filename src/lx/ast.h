/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef LX_AST_H
#define LX_AST_H

struct fsm;
struct var;

struct ast_mapping {
	struct fsm *fsm;
	struct ast_token *token;
	struct ast_zone  *to;
	struct mapping_set *conflict;

	struct ast_mapping *next;
};

struct ast_zone {
	struct ast_mapping *ml;
	struct fsm *fsm;
	struct var *vl;

	struct ast_zone *parent;
	struct ast_zone *next;
};

struct ast_token {
	const char *s;

	struct ast_token *next;
};

struct mapping_set {
	struct ast_mapping *m;
	struct mapping_set *next;
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
ast_addzone(struct ast *ast, struct ast_zone *parent);

struct ast_mapping *
ast_addmapping(struct ast_zone *z, struct fsm *fsm,
	struct ast_token *token, struct ast_zone *to);

struct mapping_set *
ast_addconflict(struct mapping_set **head, struct ast_mapping *n);

int
ast_setendmapping(struct fsm *fsm, struct ast_mapping *m);

struct ast_mapping *
ast_getendmappingbyendid(fsm_end_id_t id);

struct ast_mapping *
ast_getendmapping(const struct fsm *fsm, fsm_state_t s);

#endif

