/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_PRIQ_H
#define ADT_PRIQ_H

struct fsm_alloc;
struct fsm_state;

struct priq {
	struct priq *next;
	unsigned cost;

	/* XXX: specific to shortest.c */
	fsm_state_t state;
	struct priq *prev; /* previous node in shortest path */
	char c;
};

/*
 * Pop the minimum cost node.
 */
struct priq *
priq_pop(struct priq **priq);

/*
 * Enqueue with priority.
 */
struct priq *
priq_push(const struct fsm_alloc *a, struct priq **priq,
	fsm_state_t state, unsigned int cost);

void
priq_update(struct priq **priq, struct priq *s, unsigned int cost);

/* XXX: set operation */
struct priq *
priq_find(struct priq *priq, fsm_state_t state);

/* XXX: set operation */
void
priq_move(struct priq **priq, struct priq *new);

/* XXX: set operation */
void
priq_free(const struct fsm_alloc *a, struct priq *priq);

#endif

