/* $Id$ */

#ifndef FSM_H
#define FSM_H

struct fsm;
struct fsm_state;
struct fsm_edge;

struct fsm_options {
	/* boolean: true indicates to omit names for states in output */
	int anonymous_states:1;
};

/*
 * Create a new FSM. This is to be freed with fsm_free(). A structure allocated
 * from fsm_new() is expected to be passed as the "fsm" argument to the
 * functions in this API.
 *
 * Returns NULL on error; see errno.
 */
struct fsm *
fsm_new(void);

/*
 * Free a structure created by fsm_new(), and all of its contents.
 * No other pointers returned by this API are to be freed individually.
 */
void
fsm_free(struct fsm *fsm);

/*
 * Copy an FSM and its contents.
 *
 * Returns NULL on error; see errno.
 */
struct fsm *
fsm_copy(struct fsm *fsm);

/*
 * Copy the contents of src over dst, and free src.
 */
void
fsm_move(struct fsm *dst, struct fsm *src);

/*
 * Add a state of the given id. An existing state is returned if the id is
 * already present.
 *
 * The special id 0 may be used to indicate a state for which libfsm is to
 * invent an arbitary ID during creation. The ID invented will not be used by
 * any existing states.
 *
 * Returns NULL on error; see errno.
 */
struct fsm_state *
fsm_addstate(struct fsm *fsm, unsigned int id);

/*
 * Add an edge from a given state to a given state, labelled with the given
 * label. If an edge to that state of the same label already exists, the
 * existing edge is returned.
 *
 * The label may be NULL for an epsilon transition. Empty labels are not
 * legal.
 *
 * If non-NULL, the contents of the label are duplicated and stored
 * internally. Therefore the memory passed may be deallocated after a call
 * to fsm_addedge().
 *
 * Returns NULL on error; see errno.
 */
struct fsm_edge *
fsm_addedge(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	const char *label);


/*
 * A local copy of *options is taken, so its storage needn't remain around
 * after a call to fsm_setoptions().
 */
void
fsm_setoptions(struct fsm *fsm, const struct fsm_options *options);

/*
 * Mark a given state as being an end state or not. The value of end is treated
 * as a boolean; if zero, the state is not an end state. If non-zero, the state
 * is marked as an end state, and the value is used to distinguish between
 * various different end states during execution of the FSM. The maximum value
 * permitted is INT_MAX.
 */
void
fsm_setend(struct fsm *fsm, struct fsm_state *state, unsigned int end);

/*
 * Return if a given state is an end state.
 */
int
fsm_isend(const struct fsm *fsm, const struct fsm_state *state);

/*
 * Return true if a given FSM has an end state.
 */
int
fsm_hasend(const struct fsm *fsm);

/*
 * Register a given state as the start state for an FSM. There may only be one
 * start state; this assignment displaces a previous start-state, if a previous
 * start state exists.
 */
void
fsm_setstart(struct fsm *fsm, struct fsm_state *state);

/*
 * Return the start state for an FSM. Returns NULL if no start state is set.
 */
struct fsm_state *
fsm_getstart(const struct fsm *fsm);

/*
 * Find a state by its id. Returns NULL if no start of the given id exists.
 *
 * The id may not be 0, which is a special value reserved for use by
 * fsm_addstate().
 */
struct fsm_state *
fsm_getstatebyid(const struct fsm *fsm, unsigned int id);

#endif

