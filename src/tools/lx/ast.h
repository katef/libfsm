/* $Id$ */

#ifndef LX_AST_H
#define LX_AST_H


#include <re/re.h>


struct lx_mapping;
struct lx_zone;
struct lx_ast;


struct lx_ast *
ast_new(void);

struct lx_zone *
ast_addzone(struct lx_ast *ast);

struct lx_mapping *
ast_addmapping(struct lx_zone *z, struct re *re, const char *token);


#endif

