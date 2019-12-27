/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_MAPPINGHASHSET_H
#define ADT_MAPPINGHASHSET_H

struct fsm_alloc;
struct mapping_hashset;
struct mapping;

struct mapping_hashset_iter {
	struct hashset_iter iter;
};

struct mapping_hashset *
mapping_hashset_create(const struct fsm_alloc *a,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b));

void
mapping_hashset_free(struct mapping_hashset *hashset);

struct mapping *
mapping_hashset_add(struct mapping_hashset *hashset, struct mapping *item);

struct mapping *
mapping_hashset_find(const struct mapping_hashset *hashset, const struct mapping *item);

struct mapping *
mapping_hashset_first(const struct mapping_hashset *hashset, struct mapping_hashset_iter *it);

struct mapping *
mapping_hashset_next(struct mapping_hashset_iter *it);

#endif

