/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

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
struct fsm_options;
struct path; /* XXX */

/*
 * States in libfsm are referred to by a 0-based numeric index.
 * This is an unsigned integer type - exactly which type is
 * supposed to be private, and may be changed.
 */
typedef unsigned int fsm_state_t;

/*
 * Create a new FSM. This is to be freed with fsm_free(). A structure allocated
 * from fsm_new() is expected to be passed as the "fsm" argument to the
 * functions in this API.
 *
 * An options pointer may be passed for control over various details of
 * FSM construction and output. This may be NULL, in which case default
 * options are used.
 * When non-NULL, the storage pointed to must remain extant until fsm_free().
 *
 * Returns NULL on error; see errno.
 * TODO: perhaps automatically create a start state, and never have an empty FSM
 * TODO: also fsm_parse should create an FSM, not add into an existing one
 */
struct fsm *
fsm_new(const struct fsm_options *opt);

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

/* Returns the options of an FSM */
const struct fsm_options *
fsm_getoptions(const struct fsm *fsm);

/* Sets the options of an FSM */
void
fsm_setoptions(struct fsm *fsm, const struct fsm_options *opts);

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
fsm_merge(struct fsm *a, struct fsm *b,
	fsm_state_t *base_a, fsm_state_t *base_b);

/*
 * Add a state.
 *
 * Returns 1 on success, or 0 on error; see errno.
 */
int
fsm_addstate(struct fsm *fsm, fsm_state_t *state);

/*
 * Add multiple states.
 */
int
fsm_addstate_bulk(struct fsm *fsm, size_t n);

 /*
 * Remove a state. Any edges transitioning to this state are also removed.
 */
void
fsm_removestate(struct fsm *fsm, fsm_state_t state);

/* Use the state passed in via opaque to determine whether the state[id]
 * will have a new ID <= the current ID or be removed (in which case,
 * the callback should return FSM_STATE_REMAP_NO_STATE).
 *
 * Whether this can be used to combine states or just compact them
 * depends on the caller. */
#define FSM_STATE_REMAP_NO_STATE ((fsm_state_t)-1)
typedef fsm_state_t
fsm_state_remap_fun(fsm_state_t id, const void *opaque);

/* Use the state passed in via opaque to determine whether
 * to keep state[id]. */
typedef int
fsm_state_filter_fun(fsm_state_t id, void *opaque);

/* Rewrite the FSM by eliminating states that should be
 * removed (according to the remap callback) and then
 * compacting the rest.
 *
 * This cannot be used to combine multiple states. */
int
fsm_compact_states(struct fsm *fsm,
    fsm_state_filter_fun *filter, void *opaque,
    size_t *removed);

/*
 * Add an edge from a given state to a given state, labelled with the given
 * label. If an edge to that state of the same label already exists, the
 * existing edge is used instead, and a new edge is not added.
 *
 * Edges may be one of the following types:
 *
 * - An epsilon transition.
 * - Any character
 * - A literal character. The character '\0' is permitted.
 *
 * Returns 1 on success, or 0 on error; see errno.
 */
int
fsm_addedge_epsilon(struct fsm *fsm, fsm_state_t from, fsm_state_t to);

int
fsm_addedge_any(struct fsm *fsm, fsm_state_t from, fsm_state_t to);

int
fsm_addedge_literal(struct fsm *fsm, fsm_state_t from, fsm_state_t to,
	char c);

/*
 * Find the mode (as in median, mode and average) of the set of states to which
 * all edges from a given state transition. This is not stable (as in sort order
 * stability); if two desination states have equal frequency, then one is chosen
 * arbitrarily to be considered the mode.
 *
 * If non-NULL, the mode's frequency is written out to the freq pointer.
 *
 * A target state is only considered as a mode if it has more than one occurance.
 * Returns NULL if there is no mode (e.g. the given state has no edges),
 * and *freq is unchanged.
 */
fsm_state_t
fsm_findmode(const struct fsm *fsm, fsm_state_t state, unsigned int *freq);

/*
 * Mark a given state as being an end state or not. The value of end is treated
 * as a boolean; if zero, the state is not an end state. If non-zero, the state
 * is marked as an end state.
 */
void
fsm_setend(struct fsm *fsm, fsm_state_t state, int end);

/*
 * Set data associated with all end states.
 */
void
fsm_setendopaque(struct fsm *fsm, void *opaque);

/*
 * Set data associated with an end state.
 */
void
fsm_setopaque(struct fsm *fsm, fsm_state_t state, void *opaque);

/*
 * Get data associated with an end state.
 */
void *
fsm_getopaque(const struct fsm *fsm, fsm_state_t state);

/*
 * Find the state (if there is just one), or add epsilon edges from all states,
 * for which the given predicate is true.
 *
 * Returns 1 on success, or 0 on error; see errno.
 */
int
fsm_collate(struct fsm *fsm, fsm_state_t *state,
	int (*predicate)(const struct fsm *, fsm_state_t));

void
fsm_clearstart(struct fsm *fsm);

/*
 * Register a given state as the start state for an FSM. There may only be one
 * start state; this assignment displaces a previous start-state, if a previous
 * start state exists.
 */
void
fsm_setstart(struct fsm *fsm, fsm_state_t state);

/*
 * Find the start state for an FSM.
 * Returns 1 on success, or 0 if no start state is set.
 */
int
fsm_getstart(const struct fsm *fsm, fsm_state_t *start);

/*
 * Returns the number of states in the FSM
 */
unsigned int
fsm_countstates(const struct fsm *fsm);

/*
 * Returns the number of edges in the FSM
 */
unsigned int
fsm_countedges(const struct fsm *fsm);

/*
 * Merge two states. A new state is output to q.
 *
 * Formally, this defines an _equivalence relation_ between the two states.
 * These are joined and the language of the resulting FSM is a superset of
 * the given FSM; the FSM produced is a _quotient automaton_.
 *
 * Since the NFA in libfsm are in general Thompson NFA (i.e. with epsilon
 * transitions), merging states here preserves their epsilon edges. This
 * differs from Hopcroft & Ullman's definition of an equivalence relation,
 * which is done wrt the power set of each state.
 *
 * Returns 1 on success, or 0 on error; see errno.
 */
int
fsm_mergestates(struct fsm *fsm, fsm_state_t a, fsm_state_t b,
	fsm_state_t *q);

/*
 * Trim away "dead" states. More formally, this recursively removes
 * unreachable states (i.e. those in a subgraph which is disjoint from
 * the state state's connected component), and optionally non-end
 * states that do not have a path to an end state.
 *
 * If mode == FSM_TRIM_START_AND_END_REACHABLE and shortest_end_depth is
 * non-NULL, then allocate and write an array with the length of the
 * shortest distance to an end state for each state (post-trim) into
 * *shortest_end_distance. Since checking for paths to an end state
 * already does most of the work, this can be calculated at the same
 * time with little overhead, and some operations (such as minimising)
 * can make use of that information. On error, this array will
 * automatically be freed.
 *
 * Returns how many states were removed, or -1 on error.
 */
enum fsm_trim_mode {
	/* Remove states unreachable from the start state. */
	FSM_TRIM_START_REACHABLE,
	/* Also remove states without a path to an end state. */
	FSM_TRIM_START_AND_END_REACHABLE
};
long
fsm_trim(struct fsm *fsm, enum fsm_trim_mode mode,
	unsigned **shortest_end_distance);

/*
 * Produce a short legible string that matches up to a goal state.
 *
 * The given FSM is expected to be a Glushkov NFA.
 */
int
fsm_example(const struct fsm *fsm, fsm_state_t goal,
	char *buf, size_t bufsz);

/*
 * Reverse the given fsm. This may result in an NFA.
 *
 * Returns 1 on success, or 0 on error.
 */
int
fsm_reverse(struct fsm *fsm);

/*
 * Convert an NFA with epsilon transitions to a Glushkov NFA (NFA without
 * epsilon transitions).
 *
 * Returns false on error; see errno.
 */
int
fsm_glushkovise(struct fsm *fsm);

/*
 * Convert an fsm to a DFA.
 *
 * Returns false on error; see errno.
 */
int
fsm_determinise(struct fsm *fsm);

/*
 * Make a DFA complete, as per fsm_iscomplete.
 */
int
fsm_complete(struct fsm *fsm,
	int (*predicate)(const struct fsm *, fsm_state_t));

/*
 * Minimize a DFA to its canonical form.
 *
 * Returns false on error; see errno.
 */
int
fsm_minimise(struct fsm *fsm);

/*
 * Concatenate b after a. This is not commutative.
 */
struct fsm *
fsm_concat(struct fsm *a, struct fsm *b);

/*
 * Return 1 if the fsm does not match anything;
 * 0 if anything matches (including the empty string),
 * or -1 on error.
 */
int
fsm_empty(const struct fsm *fsm);

/*
 * Return 1 if two fsm are equal, 0 if they're not,
 * or -1 on error.
 */
int
fsm_equal(const struct fsm *a, const struct fsm *b);

/*
 * Find the least-cost ("shortest") path between two states.
 *
 * A discovered path is returned, or NULL on error. If the goal is not
 * reachable, then the path returned will be non-NULL but will not contain
 * the goal state.
 *
 * The given FSM is expected to be a Glushkov NFA.
 */
struct path *
fsm_shortest(const struct fsm *fsm,
	fsm_state_t start, fsm_state_t goal,
	unsigned (*cost)(fsm_state_t from, fsm_state_t to, char c));

/*
 * Execute an FSM reading input from the user-specified callback fsm_getc().
 * fsm_getc() is passed the opaque pointer given, and is expected to return
 * either an unsigned character cast to int, or EOF to indicate the end of
 * input.
 *
 * Returns 1 on success, or 0 for unexpected input, premature EOF, or ending
 * in a state not marked as an end state.
 * Returns -1 on error; see errno.
 *
 * On success, outputs the accepting state on a successful parse (i.e. where
 * execution ends in an state set by fsm_setend).
 *
 * The returned accepting state is intended to facillitate lookup of
 * its state opaque value previously set by fsm_setopaque() (not to be
 * confused with the opaque pointer passed for the fsm_getc callback function).
 *
 * The given FSM is expected to be a DFA.
 */
int
fsm_exec(const struct fsm *fsm, int (*fsm_getc)(void *opaque), void *opaque,
	fsm_state_t *end);

/*
 * Callbacks which may be passed to fsm_exec(). These are conveniences for
 * common situations; they could equally well be user-defined.
 *
 *  fsm_sgetc - To read from a string. Pass the address of a pointer to the
 *              first element of a string:
 *
 *                const char *s = "abc";
 *                fsm_exec(fsm, fsm_sgetc, &s);
 *
 *              Where s will be incremented to point to each character in turn.
 *
 *  fsm_fgetc - To read from a file. Pass a FILE *:
 *                fsm_exec(fsm, fsm_fgetc, stdin);
 */
int fsm_sgetc(void *opaque); /* expects opaque to be char ** */
int fsm_fgetc(void *opaque); /* expects opaque to be FILE *  */

#endif

