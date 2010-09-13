/* $Id$ */

#ifndef FSM_H
#define FSM_H

/*
 * TODO: This API needs quite some refactoring. Mostly we ought to operate
 * in-place, else the user would only free() everything. Having an explicit
 * copy interface leaves the option for duplicating, if they wish. However be
 * careful about leaving things in an indeterminate state; everything ought to
 * be atomic.
 */

struct fsm;
struct fsm_state;
struct fsm_edge;

struct fsm_options {
	/* boolean: true indicates to omit names for states in output */
	unsigned int anonymous_states:1;

	/* boolean: true indicates to optmise aesthetically during output by
	 * consolidating similar edges, and outputting a single edge with a more
	 * concise label. */
	unsigned int consolidate_edges:1;
};

/*
 * Create a new FSM. This is to be freed with fsm_free(). A structure allocated
 * from fsm_new() is expected to be passed as the "fsm" argument to the
 * functions in this API.
 *
 * Returns NULL on error; see errno.
 * TODO: perhaps automatically create a start state, and never have an empty FSM
 * TODO: also fsm_parse should create an FSM, not add into an existing one
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
 *
 * TODO: I don't really like this sort of interface. Reconsider?
 */
void
fsm_move(struct fsm *dst, struct fsm *src);

/*
 * Merge the contents of src into dst by union, and free src.
 * Returns 0 on error.
 *
 * If opaque is non-NULL, all states in src are set as if by
 * fsm_addopaque().
 *
 * TODO: I don't really like this sort of interface. Reconsider?
 */
int
fsm_union(struct fsm *dst, struct fsm *src, void *opaque);

/*
 * Add a state.
 *
 * Returns NULL on error; see errno.
 */
struct fsm_state *
fsm_addstate(struct fsm *fsm);

/*
 * Add an edge from a given state to a given state, labelled with the given
 * label. If an edge to that state of the same label already exists, the
 * existing edge is returned.
 *
 * Edges may be one of the following types:
 *
 * - An epsilon transition.
 * - Any character
 * - A literal character. The character '\0' is permitted.
 * - A human-readable label. Empty labels are not legal, and the label may
 *   not be NULL. The contents of the label are duplicated and stored
 *   internally. Therefore the memory passed may be deallocated after a call
 *   to fsm_addedge_label().
 *
 * Returns false on error; see errno.
 */
int
fsm_addedge_epsilon(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to);

int
fsm_addedge_any(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to);

int
fsm_addedge_label(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	const char *label);

int
fsm_addedge_literal(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	char c);

/*
 * A local copy of *options is taken, so its storage needn't remain around
 * after a call to fsm_setoptions().
 */
void
fsm_setoptions(struct fsm *fsm, const struct fsm_options *options);

/*
 * Mark a given state as being an end state or not. The value of end is treated
 * as a boolean; if zero, the state is not an end state. If non-zero, the state
 * is marked as an end state.
 */
void
fsm_setend(struct fsm *fsm, struct fsm_state *state, int end);

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
 * Store and retrieve user-specified opaque data per-state. Multiple opaque
 * values may be stored. Duplicate values are disregarded and silently succeed.
 *
 * Caveats apply during graph operations (such as minimization and conversion
 * from NFA to DFA); when states are to be merged to produce one output state,
 * all their opaque values must be identical, if non-NULL. If not, those
 * processes will fail; see <fsm/graph.h> for details.
 *
 * Returns false on error; see errno.
 */
int
fsm_addopaque(struct fsm *fsm, struct fsm_state *state, void *opaque);

#endif

