/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_EDGESET_H
#define ADT_EDGESET_H

struct set;
struct fsm_alloc;
struct fsm_edge;
struct edge_set;

struct edge_iter {
	struct set_iter iter;
};

struct edge_set *
edge_set_create(const struct fsm_alloc *a,
	int (*cmp)(const void *a, const void *b));

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
edge_set_firstafter(const struct edge_set *set, struct edge_iter *it, const struct fsm_edge *e);

struct fsm_edge *
edge_set_next(struct edge_iter *it);

int
edge_set_hasnext(struct edge_iter *it);

int
edge_set_empty(const struct edge_set *s);

struct fsm_edge *
edge_set_only(const struct edge_set *s);

#endif

