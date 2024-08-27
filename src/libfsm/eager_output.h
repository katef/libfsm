#ifndef EAGER_OUTPUT_H
#define EAGER_OUTPUT_H

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

struct eager_output_info;

int
fsm_eager_output_init(struct fsm *fsm);

void
fsm_eager_output_free(struct fsm *fsm);

bool
fsm_eager_output_has_eager_output(const struct fsm *fsm);

bool
fsm_eager_output_state_has_eager_output(const struct fsm *fsm, fsm_state_t state);

void
fsm_eager_output_dump(FILE *f, const struct fsm *fsm);

/* Callback for fsm_eager_output_iter_*.
 * The return value indicates whether iteration should continue.
 * The results may not be sorted in any particular order. */
typedef int
fsm_eager_output_iter_cb(fsm_state_t state, fsm_output_id_t id, void *opaque);

void
fsm_eager_output_iter_state(const struct fsm *fsm,
    fsm_state_t state, fsm_eager_output_iter_cb *cb, void *opaque);

void
fsm_eager_output_iter_all(const struct fsm *fsm,
    fsm_eager_output_iter_cb *cb, void *opaque);

bool
fsm_eager_output_has_any(const struct fsm *fsm,
    fsm_state_t state, size_t *count);

int
fsm_eager_output_compact(struct fsm *fsm, fsm_state_t *mapping, size_t mapping_count);

#endif
