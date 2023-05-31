/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_CAPTURE_H
#define FSM_CAPTURE_H

#include <stdlib.h>
#include <fsm/fsm.h>

/*
 * Offsets for a capturing match group. The first position is the
 * offset to the start of the match within the string, the second
 * position is after the end. pos[1] will always be >= pos[0].
 * The match length is equal to pos[1] - pos[0], and for a
 * zero-character match, pos[0] == pos[1].
 *
 * If there is no match, both will be set to FSM_CAPTURE_NO_POS. */
#define FSM_CAPTURE_NO_POS ((size_t)-1)
struct fsm_capture {
	size_t pos[2];
};

/* What is the max capture ID an FSM uses? */
unsigned
fsm_capture_ceiling(const struct fsm *fsm);

/* Does a specific state have any capture actions? */
int
fsm_capture_has_capture_actions(const struct fsm *fsm, fsm_state_t state);

/* Allocate a capture buffer with enough space for
 * the current FSM's captures.
 *
 * This is provided for convenience -- the necessary array
 * count can be checked with fsm_capture_ceiling, and then
 * the buffer can be allocated directly. */
struct fsm_capture *
fsm_capture_alloc_capture_buffer(const struct fsm *fsm);

/* Free a capture buffer. */
void
fsm_capture_free_capture_buffer(const struct fsm *fsm, struct fsm_capture *capture_buffer);

/* Note that a capture is active for a particular end state.
 * Using this for a non-end state is an unchecked error. */
int
fsm_capture_set_active_for_end(struct fsm *fsm,
    unsigned capture_id, fsm_state_t end_state);

#ifndef NDEBUG
#include <stdio.h>

/* Dump capture metadata about an FSM. */
void
fsm_capture_dump(FILE *f, const char *tag, const struct fsm *fsm);
#endif

#endif
