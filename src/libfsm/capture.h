#ifndef LIBFSM_CAPTURE_H
#define LIBFSM_CAPTURE_H

#include <stdlib.h>
#include <fsm/fsm.h>
#include <fsm/capture.h>

#define NEXT_STATE_END ((fsm_state_t)-1)

#define CAPTURE_NO_STATE ((fsm_state_t)-1)

/* Capture interface -- functions internal to libfsm.
 * The public interface should not depend on any of these details. */

enum capture_action_type {
	/* Start an active capture if transitioning to TO. */
	CAPTURE_ACTION_START,
	/* Continue an active capture if transitioning to TO,
	 * otherwise deactivate it. */
	CAPTURE_ACTION_EXTEND,
	/* Write a zero-step capture (i.e., the start and
	 * end state are the same). */
	CAPTURE_ACTION_COMMIT_ZERO_STEP,
	/* Write an active capture's endpoints. */
	CAPTURE_ACTION_COMMIT
};

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

/* Add a capture action. This is used to update capture actions
 * in the destination FSM when combining/transforming other FSMs. */
int
fsm_capture_add_action(struct fsm *fsm,
    fsm_state_t state, enum capture_action_type type,
    unsigned id, fsm_state_t to);

/* Callback for iterating over capture actions.
 * Return 1 to continue, return 0 to halt.
 * If TO is not meaningful for a particular type, it will be
 * set to NEXT_STATE_END. */
typedef int
fsm_capture_action_iter_cb(fsm_state_t state,
    enum capture_action_type type, unsigned capture_id, fsm_state_t to,
    void *opaque);

void
fsm_capture_action_iter(const struct fsm *fsm,
    fsm_capture_action_iter_cb *cb, void *opaque);

extern const char *fsm_capture_action_type_name[];

#endif
