#ifndef ENDIDS_H
#define ENDIDS_H

#include <stdlib.h>
#include <fsm/fsm.h>

int
fsm_endid_init(struct fsm *fsm);

void
fsm_endid_free(struct fsm *fsm);

enum fsm_endid_set_res {
	FSM_ENDID_SET_ADDED,
	FSM_ENDID_SET_ALREADY_PRESENT,
	FSM_ENDID_SET_ERROR_ALLOC_FAIL = -1
};
enum fsm_endid_set_res
fsm_endid_set(struct fsm *fsm,
    fsm_state_t state, fsm_end_id_t id);

/* Sets end ids in a bulk operation.
 *
 * Repeatedly calling fsm_endid_set will end up with quadratic behavior.  This
 * routine avoids that by setting the ids all at once, and then sorting and
 * removing duplicates.
 *
 * ids may contain duplicates and do not need to be sorted.  After ids are added,
 * fsm_endid_set_all will sort the ids and remove any duplicates.
 *
 * If state already has endids set and op == FSM_ENDID_BULK_REPLACE, then this
 * will replace the states.  Otherwise, appends all of the states to the
 * current list, removing duplicates.
 *
 * The caller maintains ownership of ids, and must free it if needed.
 *
 * Only FSM_ENDID_SET_ADDED and FSM_ENDID_SET_ERROR_ALLOC_FAIL
 * are returned.
 */
enum fsm_endid_bulk_op {
	FSM_ENDID_BULK_REPLACE = 0,
	FSM_ENDID_BULK_APPEND  = 1,
};
enum fsm_endid_set_res
fsm_endid_set_bulk(struct fsm *fsm,
    fsm_state_t state, size_t num_ids, const fsm_end_id_t *ids, enum fsm_endid_bulk_op op);

size_t
fsm_endid_count(const struct fsm *fsm,
    fsm_state_t state);

enum fsm_getendids_res
fsm_endid_get(const struct fsm *fsm, fsm_state_t end_state,
    size_t id_buf_count, fsm_end_id_t *id_buf,
    size_t *ids_written);

int
fsm_endid_carry(const struct fsm *src_fsm, const struct state_set *src_set,
    struct fsm *dst_fsm, fsm_state_t dst_state);

/* Callback when iterating over the endids.
 * Return 0 to halt, or non-zero to continue. */
typedef int
fsm_endid_iter_cb(fsm_state_t state, const fsm_end_id_t id, void *opaque);

void
fsm_endid_iter(const struct fsm *fsm,
    fsm_endid_iter_cb *cb, void *opaque);

/* Callback when bulk iterating over the endids.
 * Return 0 to halt, or non-zero to continue. */
typedef int
fsm_endid_iter_bulk_cb(fsm_state_t state, const fsm_end_id_t *ids, size_t num_ids, void *opaque);

/* returns 0 if the callback ever returns 0, otherwise returns 1 */
int
fsm_endid_iter_bulk(const struct fsm *fsm,
    fsm_endid_iter_bulk_cb *cb, void *opaque);

/* Same as fsm_endid_iter, but only for a single state. */
void
fsm_endid_iter_state(const struct fsm *fsm, fsm_state_t state,
    fsm_endid_iter_cb *cb, void *opaque);

void
fsm_endid_dump(FILE *f, const struct fsm *fsm);

#endif
