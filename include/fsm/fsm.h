/* $Id$ */

#ifndef FSM_H
#define FSM_H

/*
 * TODO: This API needs quite some refactoring. Mostly we ought to operate
 * in-place, else the user would only free() everything. Having an explicit
 * clone interface leaves the option for duplicating, if they wish. However be
 * careful about leaving things in an indeterminate state; everything ought to
 * be atomic.
 */

struct fsm;
struct fsm_state;
struct fsm_edge;

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
 * Duplicate an FSM.
 */
struct fsm *
fsm_clone(const struct fsm *fsm);

/*
 * Copy the contents of src over dst, and free src.
 *
 * TODO: I don't really like this sort of interface. Reconsider?
 */
void
fsm_move(struct fsm *dst, struct fsm *src);

/*
 * Merge states from a and b. This is a memory-management operation only;
 * the storage for the two sets of states is combined, but no edges are added.
 *
 * The resulting FSM has two disjoint sets, and no start state.
 * Cannot return NULL.
 */
struct fsm *
fsm_merge(struct fsm *a, struct fsm *b);

/*
 * Add a state.
 *
 * Returns NULL on error; see errno.
 */
struct fsm_state *
fsm_addstate(struct fsm *fsm);

 /*
 * Remove a state. Any edges transitioning to this state are also removed.
 */
void
fsm_removestate(struct fsm *fsm, struct fsm_state *state);

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
struct fsm_edge *
fsm_addedge_epsilon(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to);

struct fsm_edge *
fsm_addedge_any(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to);

struct fsm_edge *
fsm_addedge_label(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	const char *label);

struct fsm_edge *
fsm_addedge_literal(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	char c);

/*
 * Find the mode (as in median, mode and average) of the set of states to which
 * all edges from a given state transition. This is not stable (as in sort order
 * stability); if two desination states have equal frequency, then one is chosen
 * arbitrarily to be considered the mode.
 *
 * A target state is only considered as a mode if it has more than one occurance.
 * Returns NULL if there is no mode (e.g. the given state has no edges).
 */
struct fsm_state *
fsm_findmode(const struct fsm_state *state);

/*
 * Mark a given state as being an end state or not. The value of end is treated
 * as a boolean; if zero, the state is not an end state. If non-zero, the state
 * is marked as an end state.
 */
void
fsm_setend(struct fsm *fsm, struct fsm_state *state, int end);

/*
 * Return true if a given FSM has an end state.
 */
int
fsm_hasend(const struct fsm *fsm);

/*
 * Return state (if there is just one), or add epsilon edges from all states,
 * for which the given predicate is true.
 *
 * Returns NULL on error, or if there is no state.
 */
struct fsm_state *
fsm_collate(struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *));

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
 * Duplicate a state and all its targets. This traverses the edges leading out
 * from the state, to the states to which those point, and so on. It does not
 * include edges going *to* the given state, or its descendants.
 *
 * For duplicate_subgraphbetween(), if non-NULL, the state x in the origional
 * subgraph is overwritten with its equivalent state in the duplicated subgraph.
 * This provides a mechanism to keep track of a state of interest (for example
 * the endpoint of a segment).
 *
 * Returns the duplicated state, or NULL on error; see errno.
 *
 * TODO: fsm_state_duplicatesubgraphx() is a horrible inteface, but I'm not
 * sure how else to go about that.
 */
struct fsm_state *
fsm_state_duplicatesubgraph(struct fsm *fsm, struct fsm_state *state);
struct fsm_state *
fsm_state_duplicatesubgraphx(struct fsm *fsm, struct fsm_state *state,
	struct fsm_state **x);

#endif

