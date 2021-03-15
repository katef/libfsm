#ifndef IPRIQ_H
#define IPRIQ_H

#include <stdlib.h>

#include <adt/alloc.h>

/* Opaque index priority queue handle. */
struct ipriq;

/* Comparison function for two indices, which may refer
 * to elements in the opaque structure. */
enum ipriq_cmp_res {
	IPRIQ_CMP_LT = -1,
	IPRIQ_CMP_EQ = 0,
	IPRIQ_CMP_GT = 1,
};
typedef enum ipriq_cmp_res
ipriq_cmp_fun(size_t a, size_t b, void *opaque);

/* Allocate a new ipriq handle. Returns NULL on error.
 *
 * alloc and opaque can be NULL, cmp must be non-NULL. */
struct ipriq *
ipriq_new(const struct fsm_alloc *alloc,
    ipriq_cmp_fun *cmp, void *opaque);

void
ipriq_free(struct ipriq *pq);

int
ipriq_empty(const struct ipriq *pq);

int
ipriq_peek(const struct ipriq *pq, size_t *res);

int
ipriq_pop(struct ipriq *pq, size_t *res);

int
ipriq_add(struct ipriq *pq, size_t x);

#endif
