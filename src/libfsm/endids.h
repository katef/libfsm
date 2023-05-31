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

int
fsm_endid_compact(struct fsm *fsm,
    const fsm_state_t *mapping, size_t mapping_count);

/* Callback when iterating over the endids.
 * Return 0 to halt, or non-zero to continue. */
typedef int
fsm_endid_iter_cb(const struct fsm *fsm, fsm_state_t state,
    size_t nth, const fsm_end_id_t id, void *opaque);

void
fsm_endid_iter(const struct fsm *fsm,
    fsm_endid_iter_cb *cb, void *opaque);

/* Same as fsm_endid_iter, but only for a single state. */
void
fsm_endid_iter_state(const struct fsm *fsm, fsm_state_t state,
    fsm_endid_iter_cb *cb, void *opaque);

void
fsm_endid_dump(FILE *f, const struct fsm *fsm);

#endif
