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
struct fsm_state;
struct fsm_edge;
struct fsm_determinise_cache;
struct fsm_options;
struct set; /* XXX */
struct path; /* XXX */

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
 * Internal free function that invokes free(3) by default, or a user-provided
 * free function to free memory and perform any custom memory tracking or handling
 */
void
f_free(const struct fsm *fsm, void *p);

/*
 * Internal malloc function that invokes malloc(3) by default, or a user-provided
 * malloc function to allocate memory and perform any custom memory tracking or handling
 */
void *
f_malloc(const struct fsm *fsm, size_t sz);

/*
 * Internal realloc function that invokes realloc(3) by default, or a user-provided
 * realloc function to re-allocate memory to the specified size and perform
 * any custom memory tracking or handling
 */
void *
f_realloc(const struct fsm *fsm, void *p, size_t sz);

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
 *
 * Returns false on error; see errno.
 */
struct fsm_edge *
fsm_addedge_epsilon(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to);

struct fsm_edge *
fsm_addedge_any(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to);

struct fsm_edge *
fsm_addedge_literal(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
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
struct fsm_state *
fsm_findmode(const struct fsm_state *state, unsigned int *freq);

/*
 * Mark a given state as being an end state or not. The value of end is treated
 * as a boolean; if zero, the state is not an end state. If non-zero, the state
 * is marked as an end state.
 */
void
fsm_setend(struct fsm *fsm, struct fsm_state *state, int end);

/*
 * Set data associated with all end states.
 */
void
fsm_setendopaque(struct fsm *fsm, void *opaque);

/*
 * Set data associated with an end state.
 */
void
fsm_setopaque(struct fsm *fsm, struct fsm_state *state, void *opaque);

/*
 * Get data associated with an end state.
 */
void *
fsm_getopaque(const struct fsm *fsm, const struct fsm_state *state);

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

/*
 * Merge two states. A new state is returned.
 *
 * Cannot return NULL.
 *
 * Formally, this defines an _equivalence relation_ between the two states.
 * These are joined and the language of the resulting FSM is a superset of
 * the given FSM; the FSM produced is a _quotient automaton_.
 *
 * Since the NFA in libfsm are in general Thompson NFA (i.e. with epsilon
 * transitions), merging states here preserves their epsilon edges. This
 * differs from Hopcroft & Ullman's definition of an equivalence relation,
 * which is done wrt the power set of each state.
 */
struct fsm_state *
fsm_mergestates(struct fsm *fsm, struct fsm_state *a, struct fsm_state *b);

/*
 * Trim away "dead" states. More formally, this recursively removes
 * unreachable states (i.e. those in a subgraph which is disjoint from
 * the state state's connected component), and non-end states which
 * do not have a path to an end state.
 */
int
fsm_trim(struct fsm *fsm);

int
fsm_example(const struct fsm *fsm, const struct fsm_state *goal,
	char *buf, size_t bufsz);

/*
 * Reverse the given fsm. This may result in an NFA.
 *
 * Returns 1 on success, or 0 on error.
 */
int
fsm_reverse(struct fsm *fsm);

/*
 * Convert an fsm to a DFA.
 *
 * Returns false on error; see errno.
 */
int
fsm_determinise(struct fsm *fsm);

/*
 * Like fsm_determinise_opaque, but allows the caller to store a pointer to
 * resources used by this API, reducing memory allocation and copy pressure.
 *
 * Free the cached data by calling fsm_determinise_freecache().
 */
int
fsm_determinise_cache(struct fsm *fsm,
	struct fsm_determinise_cache **dcache);
void
fsm_determinise_freecache(struct fsm *fsm, struct fsm_determinise_cache *dcache);

/*
 * Make a DFA complete, as per fsm_iscomplete.
 */
int
fsm_complete(struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *));

/*
 * Minimize an FSM to its canonical form.
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
 */
struct path *
fsm_shortest(const struct fsm *fsm,
	const struct fsm_state *start, const struct fsm_state *goal,
	unsigned (*cost)(const struct fsm_state *from, const struct fsm_state *to, int c));

/*
 * Execute an FSM reading input from the user-specified callback fsm_getc().
 * fsm_getc() is passed the opaque pointer given, and is expected to return
 * either an unsigned character cast to int, or EOF to indicate the end of
 * input.
 *
 * Returns the accepting state on a successful parse (i.e. where execution
 * ends in an state set by fsm_setend). Returns NULL for unexpected input,
 * premature EOF, or ending in a state not marked as an end state.
 *
 * The returned accepting state is intended to facillitate lookup of
 * its state opaque value previously set by fsm_setopaque() (not to be
 * confused with the opaque pointer passed for the fsm_getc callback function).
 *
 * The given FSM is expected to be a DFA.
 */
struct fsm_state *
fsm_exec(const struct fsm *fsm, int (*fsm_getc)(void *opaque), void *opaque);

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

