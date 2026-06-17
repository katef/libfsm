/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_INTERNAL_PRINT_H
#define RE_INTERNAL_PRINT_H

struct ast;
struct fsm_options;

enum ast_print_lang {
    AST_PRINT_NONE,
    AST_PRINT_DOT,
    AST_PRINT_ABNF,
    AST_PRINT_PCRE,
    AST_PRINT_TREE
};

// TODO: return int
typedef void ast_print_f(FILE *f, const struct fsm_options *opt,
    enum re_flags re_flags, const struct ast *ast);

ast_print_f ast_print_abnf;
ast_print_f ast_print_dot;
ast_print_f ast_print_pcre;
ast_print_f ast_print_tree;

int ast_print(FILE *f, const struct ast *ast, const struct fsm_options *opt,
    enum re_flags re_flags, enum ast_print_lang lang);

#endif
