/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>
#include <adt/hashset.h>
#include <adt/mappinghashset.h>

#include "internal.h"

/* TODO: centralise as a predicate over a state set */
static int
hasend(const struct fsm *fsm, const struct state_set *set)
{
	struct state_iter it;
	fsm_state_t s;

	assert(fsm != NULL);

	for (state_set_reset((void *) set, &it); state_set_next(&it, &s); ) {
		if (fsm_isend(fsm, s)) {
			return 1;
		}
	}

	return 0;
}

/*
 * This maps a DFA state onto its associated NFA symbol closure, such that an
 * existing DFA state may be found given any particular set of NFA states
 * forming a symbol closure.
 */
struct mapping {
	/* The set of NFA states forming the symbol closure for this DFA state */
	struct state_set *closure;

	/* The DFA state associated with this epsilon closure of NFA states */
	fsm_state_t dfastate;

	/* Newly-created DFA edges */
	struct edge_set *edges;
};

static int
cmp_mapping(const void *a, const void *b)
{
	const struct mapping * const *ma = a, * const *mb = b;

	assert(ma != NULL && *ma != NULL);
	assert(mb != NULL && *mb != NULL);

	return state_set_cmp((*ma)->closure, (*mb)->closure);
}

static unsigned long
hash_mapping(const void *a)
{
	const struct mapping *m = a;

	/* TODO: could cache the hashed value in the mapping struct,
	 * or populate it when making the mapping */
	return state_set_hash(m->closure);
}

static struct mapping *
mapping_find(const struct mapping_hashset *mappings, const struct state_set *closure)
{
	struct mapping *m, search;

	assert(mappings != NULL);
	assert(closure != NULL);

	search.closure = (void *) closure;
	m = mapping_hashset_find(mappings, &search);
	if (m != NULL) {
		return m;
	}

	return NULL;
}

/*
 * By contract an existing mapping is assumed to not exist.
 */
static struct mapping *
mapping_add(struct mapping_hashset *mappings, const struct fsm_alloc *alloc,
	fsm_state_t dfastate, struct state_set *closure)
{
	struct mapping *m;

	assert(mappings != NULL);
	assert(!mapping_find(mappings, closure));
	assert(closure != NULL);

	m = f_malloc(alloc, sizeof *m);
	if (m == NULL) {
		return NULL;
	}

	m->closure  = closure;
	m->dfastate = dfastate;
	m->edges    = NULL;

	if (!mapping_hashset_add(mappings, m)) {
		f_free(alloc, m);
		return NULL;
	}

	return m;
}

static int
mapping_addedges(struct mapping *from, struct mapping *to,
	const struct fsm_alloc *alloc,
	unsigned char c)
{
	struct fsm_edge *e;

	e = f_malloc(alloc, sizeof *e);
	if (e == NULL) {
		return 0;
	}

	e->sl     = NULL;
	e->symbol = c;

	if (!state_set_add(&e->sl, alloc, to->dfastate)) {
		goto error;
	}

	/*
	 * Note there is no looking up of an edge by symbol;
	 * TODO: this should suit the adjacency list for bulk-adding edges i hope
	 */

	if (from->edges == NULL) {
		from->edges = edge_set_create(alloc, fsm_state_cmpedges);
		if (from->edges == NULL) {
			goto error;
		}
	}

	if (!edge_set_add(from->edges, e)) {
		goto error;
	}

	return 1;

error:

	state_set_free(e->sl);
	f_free(alloc, e);

	return 0;
}

/* TODO: this stack is just a placeholder for something more suitable */
struct mappingstack {
	struct mapping *item;
	struct mappingstack *prev;
};

static int
stack_push(struct mappingstack **stack, const struct fsm_alloc *alloc,
	struct mapping *item)
{
	struct mappingstack *new;

	new = f_malloc(alloc, sizeof *new);
	if (new == NULL) {
		return 0;
	}

	new->item = item;
	new->prev = *stack;

	*stack = new;

	return 1;
}

static struct mapping *
stack_pop(struct mappingstack **stack, const struct fsm_alloc *alloc)
{
	struct mappingstack *p;
	struct mapping *m;

	if (*stack == NULL) {
		return NULL;
	}

	p = *stack;

	*stack = p->prev;

	p->prev = NULL;

	m = p->item;
	f_free(alloc, p);

	assert(m != NULL);

	return m;
}

int
fsm_determinise(struct fsm *nfa)
{
	struct state_set **eclosures;
	struct mappingstack *stack;
	struct mapping_hashset *mappings;
	struct mapping *curr;
	size_t dfacount;

	assert(nfa != NULL);

	eclosures = epsilon_closure_bulk(nfa);
	if (eclosures == NULL) {
		return 0;
	}

	dfacount = 0;

	mappings = mapping_hashset_create(nfa->opt->alloc, hash_mapping, cmp_mapping);
	if (mappings == NULL) {
		closure_free(eclosures, nfa->statecount);
		return 0;
	}

	{
		fsm_state_t start;
		struct state_set *set;

		/*
		 * The starting condition is the epsilon closure of a set of states
		 * containing just the start state.
		 *
		 * You can think of this as having reached the FSM's start state by
		 * previously consuming some imaginary prefix symbol (giving the set
		 * containing just the start state), and then this epsilon closure
		 * is equivalent to the usual case of taking the epsilon closure after
		 * each symbol closure in the main loop.
		 *
		 * (We take a copy of this set for sake of memory ownership only,
		 * for freeing the epsilon closures later).
		 */

		if (!fsm_getstart(nfa, &start)) {
			errno = EINVAL;
			/* TODO: free mappings, eclosures */
			return 0;
		}

		set = NULL;

		if (!state_set_copy(&set, nfa->opt->alloc, eclosures[start])) {
			goto error;
		}

		curr = mapping_add(mappings, nfa->opt->alloc, dfacount++, set);
		if (curr == NULL) {
			/* TODO: free mappings, eclosures, set */
			goto error;
		}
	}

	/*
	 * Our "todo" list. It needn't be a stack; we treat it as an unordered
	 * set where we can consume arbitrary items in turn.
	 */
	stack = NULL;

	do {
		struct state_set *sclosures[FSM_SIGMA_COUNT] = { NULL };
		int i;

		assert(curr != NULL);

		/* TODO: explain the closure of a set is equivalent to the union
		 * of closures of each item. So here we iteratively build up sclosures[] in-situ. */
		{
			struct state_iter it;
			fsm_state_t s;

			for (state_set_reset(curr->closure, &it); state_set_next(&it, &s); ) {
				if (!symbol_closure_bulk(nfa, s, eclosures, sclosures)) {
					/* TODO: free mappings, eclosures, sclosures, stack */
					goto error;
				}
			}
		}

		for (i = 0; i <= FSM_SIGMA_MAX; i++) {
			struct mapping *m;

			if (sclosures[i] == NULL) {
				continue;
			}

			/* TODO: explain that { sclosures[i] } is our new dfa state */
			/* TODO: explain we only really use the mappings as a de-duplication mechanism */

			/* Use an existing mapping if present, otherwise add a new one */
			m = mapping_find(mappings, sclosures[i]);
			if (m != NULL) {
				/* we already have this closure, free this instance */
				state_set_free(sclosures[i]);

				assert(m->dfastate < dfacount);
			} else {
				m = mapping_add(mappings, nfa->opt->alloc, dfacount++, sclosures[i]);
				if (m == NULL) {
					/* TODO: free mappings, eclosures, sclosures, stack */
					goto error;
				}

				/* ownership belongs to the mapping now, so don't free sclosures[i] */

				if (!stack_push(&stack, nfa->opt->alloc, m)) {
					/* TODO: free mappings, eclosures, sclosures, stack */
					goto error;
				}
			}

			if (!mapping_addedges(curr, m, nfa->opt->alloc, i)) {
				/* TODO: free mappings, eclosures, sclosures, stack */
				goto error;
			}
		}

		/* all elements in sclosures[] have been freed or moved to their
		 * respective mapping, so there's nothing to free here */
	} while (curr = stack_pop(&stack, nfa->opt->alloc), curr != NULL);

	closure_free(eclosures, nfa->statecount);

	{
		struct mapping_hashset_iter it;
		struct mapping *m;
		struct fsm *dfa;

		dfa = fsm_new(nfa->opt);
		if (dfa == NULL) {
			goto error;
		}

		/* TODO: provide bulk creation for states */
		{
			fsm_state_t dummy;
			size_t j;

			for (j = 0; j < dfacount; j++) {
				if (!fsm_addstate(dfa, &dummy)) {
					/* TODO: free stuff */
					goto error;
				}
			}
		}

		assert(dfa->statecount == dfacount);

		/*
		 * We know state 0 is the start state, because its mapping
		 * was created first.
		 */
		fsm_setstart(dfa, 0);

		for (m = mapping_hashset_first(mappings, &it); m != NULL; m = mapping_hashset_next(&it)) {
			assert(m->dfastate < dfa->statecount);
			assert(dfa->states[m->dfastate].edges == NULL);

			dfa->states[m->dfastate].edges = m->edges;

			/*
			 * The current DFA state is an end state if any of its associated NFA
			 * states are end states.
			 */

			if (!hasend(nfa, m->closure)) {
				continue;
			}

			fsm_setend(dfa, m->dfastate, 1);

			/*
			 * Carry through opaque values, if present. This isn't anything to do
			 * with the DFA conversion; it's meaningful only to the caller.
			 *
			 * The closure may contain non-end states, but at least one state is
			 * known to have been an end state.
			 */
			fsm_carryopaque(nfa, m->closure, dfa, m->dfastate);
		}

		fsm_move(nfa, dfa);
	}

	{
		struct mapping_hashset_iter it;
		struct mapping *m;

		for (m = mapping_hashset_first(mappings, &it); m != NULL; m = mapping_hashset_next(&it)) {
			state_set_free(m->closure);
			f_free(nfa->opt->alloc, m);
		}
	}

	return 1;

error:

	/* TODO: free stuff */

	return 0;
}

