#ifndef ENDIDS_H
#define ENDIDS_H

#include <stdlib.h>
#include <fsm/fsm.h>

/* Opaque. */
struct fsm_endid_info;

struct fsm_endid_info *
fsm_endid_alloc(struct fsm_alloc *alloc);

void
fsm_endid_free(struct fsm_endid_info *ei);

int
fsm_endid_set(struct fsm_endid_info *ei,
    fsm_state_t state, fsm_end_id_t id);

size_t
fsm_endid_count(const struct fsm_endid_info *ei,
    fsm_state_t state);

int
fsm_endid_get(const struct fsm_endid_info *ei, fsm_state_t end_state,
    size_t id_buf_count, fsm_end_id_t *id_buf,
    size_t *ids_written);

#endif
