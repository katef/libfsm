/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_STATESET_H
#define ADT_STATESET_H

struct set;
struct fsm_state;

struct state_set {
	struct set *set;
};

struct state_set *
state_set_create(void);

void
state_set_free(struct state_set *set);

struct fsm_state *
state_set_add(struct state_set *set, struct fsm_state *st);

void
state_set_remove(struct state_set *set, const struct fsm_state *st);

int
state_set_empty(const struct state_set *s);

struct fsm_state *
state_set_only(const struct state_set *s);

struct state_iter {
	struct set_iter iter;
};

struct fsm_state *
state_set_contains(const struct state_set *set, const struct fsm_state *st);

size_t
state_set_count(const struct state_set *set);

struct fsm_state *
state_set_first(struct state_set *set, struct state_iter *it);

struct fsm_state *
state_set_next(struct state_iter *it);

int
state_set_hasnext(struct state_iter *it);

/* uses set_cmp */
int
state_set_cmp(const struct state_set *a, const struct state_set *b);

const struct fsm_state **
state_set_array(const struct state_set *set);

#endif

