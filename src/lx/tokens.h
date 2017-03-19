/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

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

