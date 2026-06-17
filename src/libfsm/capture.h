#ifndef LIBFSM_CAPTURE_H
#define LIBFSM_CAPTURE_H

#include <stdlib.h>
#include <stdio.h>
#include <fsm/fsm.h>
#include <fsm/capture.h>
#include <adt/stateset.h>

/* Internal state IDs that are out of range for valid state IDs.
 *
 * CAPTURE_NO_STATE is used to represent the absence of a state, such as
 * when remapping a state to a dead state (removing it) or empty
 * hash table buckets.
 *
 * NEXT_STATE_END is used as a destination for capture actions that
 * trigger when ending on a state. */
#define CAPTURE_NO_STATE ((fsm_state_t)-1)

/* Capture interface -- functions internal to libfsm.
 * The public interface should not depend on any of these details. */

struct fsm_capture_info;
struct capvm_program;

int
fsm_capture_init(struct fsm *fsm);

void
fsm_capture_free(struct fsm *fsm);

void
fsm_capture_dump_active_for_ends(FILE *f, const struct fsm *fsm);

void
fsm_capture_dump_program_end_mapping(FILE *f, const struct fsm *fsm);

/* Does the FSM have captures? */
int
fsm_capture_has_captures(const struct fsm *fsm);

void
fsm_capture_dump(FILE *f, const char *tag, const struct fsm *fsm);

void
fsm_capture_dump_programs(FILE *f, const struct fsm *fsm);

/* If EXPENSIVE_CHECKS is non-zero, assert that all capture metadata on
 * an FSM is internally consistent. */
void
fsm_capture_integrity_check(const struct fsm *fsm);

int
fsm_capture_id_compact(struct fsm *fsm, const fsm_state_t *mapping,
    size_t orig_statecount);

int
fsm_capture_program_association_compact(struct fsm *fsm, const fsm_state_t *mapping,
    size_t orig_statecount);

/* Iterator callback for capture IDs that are active for a particular
 * end state. Returns whether iteration should continue. */
typedef int
fsm_capture_iter_active_for_end_cb(fsm_state_t state, unsigned capture_id,
    void *opaque);

void
fsm_capture_iter_active_for_end_state(const struct fsm *fsm, fsm_state_t state,
    fsm_capture_iter_active_for_end_cb *cb, void *opaque);

void
fsm_capture_iter_active_for_all_end_states(const struct fsm *fsm,
    fsm_capture_iter_active_for_end_cb *cb, void *opaque);

/* Iterator callback for program IDs that are active for a particular
 * end state. Returns whether iteration should continue. */
typedef int
fsm_capture_iter_program_ids_for_end_state_cb(fsm_state_t state, unsigned prog_id,
     void *opaque);

void
fsm_capture_iter_program_ids_for_end_state(const struct fsm *fsm, fsm_state_t state,
    fsm_capture_iter_program_ids_for_end_state_cb *cb, void *opaque);
void
fsm_capture_iter_program_ids_for_all_end_states(const struct fsm *fsm,
    fsm_capture_iter_program_ids_for_end_state_cb *cb, void *opaque);

/* TODO: combine/rename */
int
fsm_capture_copy_active_for_ends(const struct fsm *src_fsm,
	const struct state_set *states,
	struct fsm *dst_fsm, fsm_state_t dst_state);
int
fsm_capture_copy_program_end_state_associations(const struct fsm *src_fsm,
	const struct state_set *states,
	struct fsm *dst_fsm, fsm_state_t dst_state);

int
fsm_capture_copy_programs(const struct fsm *src_fsm,
	struct fsm *dst_fsm);

size_t
fsm_capture_program_count(const struct fsm *fsm);

void
fsm_capture_update_max_capture_id(struct fsm_capture_info *ci,
	unsigned capture_id);

int
fsm_capture_add_program(struct fsm *fsm,
	struct capvm_program *program, uint32_t *prog_id);

const struct capvm_program *
fsm_capture_get_program_by_id(const struct fsm *fsm, uint32_t prog_id);

int
fsm_capture_associate_program_with_end_state(struct fsm *fsm,
	uint32_t prog_id, fsm_state_t end_state);

/* Resolve captures.
 *
 * FIXME: With the current implementation, if enough memory
 * was passed in then it couldn't fail, but it may be worth
 * changing the interface so that it doesn't assume there was
 * already a successful match in order to support one-pass
 * matching & capture resolution attempts from a stream.
 *
 * TODO: This should pass in a size for captures[].
 * TODO: An alternate interface that allows passing in
 *       preallocated buffers for working memory.
 *
 * TODO: describe return value. */
int
fsm_capture_resolve_during_exec(const struct fsm *fsm,
	fsm_state_t end_state, const unsigned char *input, size_t input_offset,
	struct fsm_capture *captures, size_t captures_len);

#endif
