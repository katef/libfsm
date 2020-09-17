#ifndef LIBFSM_CAPTURE_H
#define LIBFSM_CAPTURE_H

#include <stdlib.h>
#include <fsm/fsm.h>
#include <fsm/capture.h>

int
fsm_capture_init(struct fsm *fsm);

void
fsm_capture_free(struct fsm *fsm);

/* Update captures, called when exiting or ending on a state.
 * If ending on a state, use NEXT_STATE_END for next_state. */
void
fsm_capture_update_captures(const struct fsm *fsm,
    fsm_state_t cur_state, fsm_state_t next_state, size_t offset,
    struct fsm_capture *captures);

void
fsm_capture_finalize_captures(const struct fsm *fsm,
    size_t capture_count, struct fsm_capture *captures);

#endif
