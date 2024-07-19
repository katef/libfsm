#ifndef FSM_PARSER_H
#define FSM_PARSER_H

struct fsm;
struct fsm_alloc;
struct fsm_options;

struct fsm *
fsm_parse(FILE *, const struct fsm_alloc *alloc, const struct fsm_options *);

#endif
