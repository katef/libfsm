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

/* How many captures does the FSM use? */
unsigned
fsm_countcaptures(const struct fsm *fsm);

/* Does a specific state have any capture actions? */
int
fsm_capture_has_capture_actions(const struct fsm *fsm, fsm_state_t state);

/* Set a capture path on an FSM. This means that during matching, the
 * portion of a match between the path's START and END states will be
 * captured. As the FSM is transformed (determinisation, minimisation,
 * unioning, etc.), the path will be converted to refer to the pair(s)
 * of new states instead. If the path's END state is no longer reachable
 * from its START state, then the capture path will be ignored.
 * Multiple instances of the same capture_id and path are ignored. */
int
fsm_capture_set_path(struct fsm *fsm, unsigned capture_id,
    fsm_state_t start, fsm_state_t end);

/* Increase the base capture ID for all captures in an fsm.
 * This could be used before combining multiple FSMs -- for
 * example, before unioning a and b, where a has 3 captures
 * and b has 2, b may be rebase'd to 3 -- so a has captures
 * 0-2 and b has 3-4. */
void
fsm_capture_rebase_capture_id(struct fsm *fsm, unsigned base);

/* Same, but for capture action states. */
void
fsm_capture_rebase_capture_action_states(struct fsm *fsm, fsm_state_t base);

/* Allocate a capture buffer with enough space for
 * the current FSM's captures. */
struct fsm_capture *
fsm_capture_alloc(const struct fsm *fsm);

#ifndef NDEBUG
#include <stdio.h>

/* Dump capture metadata about an FSM. */
void
fsm_capture_dump(FILE *f, const char *tag, const struct fsm *fsm);
#endif

#endif
