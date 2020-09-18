#ifndef RE_AST_FROM_FSM_H
#define RE_AST_FROM_FSM_H

struct fsm;
struct ast;

struct ast *
ast_new_from_fsm(const struct fsm *fsm);

#endif
