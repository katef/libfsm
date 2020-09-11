#ifndef FSM_PARSER_H
#define FSM_PARSER_H

#include <stdio.h>

struct fsm;
struct fsm_options;

struct fsm *
fsm_parse(FILE *, const struct fsm_options *);

#endif
