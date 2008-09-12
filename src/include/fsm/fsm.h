/* $Id$ */

#ifndef FSM_H
#define FSM_H

struct fsm;
struct fsm_state;
struct fsm_edge;

struct fsm_options {
	/* boolean: true indicates to omit names for states in output */
	int anonymous_states;
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
 * Add a state of the given id. An existing state is returned if the id is
 * already present.
 *
 * Returns NULL on error; see errno.
 *
 * TODO: perhaps pass 0 to have libfsm invent a state? adjust lexer accordingly
 */
struct fsm_state *
fsm_addstate(struct fsm *fsm, unsigned int id, int end);

/*
 * Add an edge from a given state to a given state, labelled with the given
 * label. If an edge to that state of the same label already exists, the
 * existing edge is returned.
 *
 * The label may be NULL for an epsilon transition. Empty labels are not
 * legal.
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
fsm_setoptions(struct fsm *fsm, struct fsm_options *options);

/*
 * Mark a given state as being an end state or not. The value of end is treated
 * as a boolean.
 */
void
fsm_setend(struct fsm *fsm, struct fsm_state *state, int end);

/*
 * Return if a given state is an end state.
 */
int
fsm_isend(const struct fsm *fsm, const struct fsm_state *state);

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
 */
struct fsm_state *
fsm_getstatebyid(const struct fsm *fsm, unsigned int id);

#endif

