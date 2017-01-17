/* $Id$ */

#ifndef LX_TOKENS_H
#define LX_TOKENS_H

struct fsm;

int
tok_contains(const struct fsm *fsm, const char *s);

int
tok_subsetof(const struct fsm *a, const struct fsm *b);

int
tok_equal(const struct fsm *a, const struct fsm *b);

#endif

