/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef EDGESET_H
#define EDGESET_H

struct set;
struct fsm_edge;

struct edge_set {
	struct set *set;
};

struct edge_iter {
	struct set_iter iter;
};

void
edge_set_free(struct edge_set *set);

struct fsm_edge *
edge_set_add(struct edge_set *set, struct fsm_edge *e);

struct fsm_edge *
edge_set_contains(const struct edge_set *set, const struct fsm_edge *e);

size_t
edge_set_count(const struct edge_set *set);

void
edge_set_remove(struct edge_set *set, const struct fsm_edge *e);

struct fsm_edge *
edge_set_first(struct edge_set *set, struct edge_iter *it);

struct fsm_edge *
edge_set_firstafter(struct edge_set *set, struct edge_iter *it, const struct fsm_edge *e);

struct fsm_edge *
edge_set_next(struct edge_iter *it);

int
edge_set_hasnext(struct edge_iter *it);

struct fsm_edge *
edge_set_only(const struct edge_set *s);

#endif

