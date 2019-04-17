/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_MAPPINGSET_H
#define ADT_MAPPINGSET_H

struct fsm_alloc;
struct mapping_set;
struct mapping;

struct mapping_iter {
	struct hashset_iter iter;
};

struct mapping_set *
mapping_set_create(const struct fsm_alloc *a,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b));

void
mapping_set_free(struct mapping_set *set);

struct mapping *
mapping_set_add(struct mapping_set *set, struct mapping *item);

struct mapping *
mapping_set_contains(const struct mapping_set *set, const struct mapping *item);

void
mapping_set_clear(struct mapping_set *set);

struct mapping *
mapping_set_first(const struct mapping_set *set, struct mapping_iter *it);

struct mapping *
mapping_set_next(struct mapping_iter *it);

#endif

