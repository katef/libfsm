#ifndef IDMAP_H
#define IDMAP_H

/* Mapping between one fsm_state_t and a set of
 * unsigned IDs. The implementation assumes that both
 * IDs are sequentially assigned and don't need a sparse
 * mapping -- it will handle 10 -> [1, 3, 47] well, but
 * not 1000000 -> [14, 524288, 1073741823]. */

#include <stdlib.h>

#include "fsm/fsm.h"
#include "fsm/alloc.h"

struct idmap;			/* Opaque handle. */

struct idmap *
idmap_new(const struct fsm_alloc *alloc);

void
idmap_free(struct idmap *m);

/* Associate a value with a state (if not already present.)
 * Returns 1 on success, or 0 on allocation failure. */
int
idmap_set(struct idmap *m, fsm_state_t state_id, unsigned value);

/* How many values are associated with an ID? */
size_t
idmap_get_value_count(const struct idmap *m, fsm_state_t state_id);

/* Get the values associated with an ID.
 *
 * Returns 1 on success and writes them into the buffer, in ascending
 * order, with the count in *written (if non-NULL).
 *
 * Returns 0 on error (insufficient buffer space). */
int
idmap_get(const struct idmap *m, fsm_state_t state_id,
	size_t buf_size, unsigned *buf, size_t *written);

/* Iterator callback. */
typedef void
idmap_iter_fun(fsm_state_t state_id, unsigned value, void *opaque);

/* Iterate over the ID map. State IDs may be yielded out of order,
 * values will be in ascending order. */
void
idmap_iter(const struct idmap *m,
	idmap_iter_fun *cb, void *opaque);

/* Iterate over the values associated with a single state
 * (in ascending order). */
void
idmap_iter_for_state(const struct idmap *m, fsm_state_t state_id,
	idmap_iter_fun *cb, void *opaque);

#endif
