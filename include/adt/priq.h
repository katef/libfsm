/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef PRIQ_H
#define PRIQ_H

struct fsm_state;

struct priq {
	struct priq *next;
	unsigned cost;

	/* XXX: specific to shortest.c */
	struct fsm_state *state;
	struct priq *prev; /* previous node in shortest path */
	int type; /* XXX: should really be fsm_edge_type */
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
priq_push(struct priq **priq,
	struct fsm_state *state, unsigned int cost);

void
priq_update(struct priq **priq, struct priq *s, unsigned int cost);

/* XXX: set operation */
struct priq *
priq_find(struct priq *priq, const struct fsm_state *state);

/* XXX: set operation */
void
priq_move(struct priq **priq, struct priq *new);

/* XXX: set operation */
void
priq_free(struct priq *priq);

#endif

