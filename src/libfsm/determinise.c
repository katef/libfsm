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
#include <fsm/walk.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>
#include <adt/hashset.h>
#include <adt/mappinghashset.h>

#include "internal.h"

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
	struct mappingstack *stack;
	struct mapping_hashset *mappings;
	struct mapping *curr;
	size_t dfacount;

	assert(nfa != NULL);

	/*
	 * This NFA->DFA implementation is for Glushkov NFA only; it keeps things
	 * a little simpler by avoiding epsilon closures here, and also a little
	 * faster where we can start with a Glushkov NFA in the first place.
	 */
	if (fsm_has(nfa, fsm_hasepsilons)) {
		if (!fsm_glushkovise(nfa)) {
			return 0;
		}
	}

	dfacount = 0;

	mappings = mapping_hashset_create(nfa->opt->alloc, hash_mapping, cmp_mapping);
	if (mappings == NULL) {
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
			/* TODO: free mappings */
			return 0;
		}

		set = NULL;

		if (!state_set_add(&set, nfa->opt->alloc, start)) {
			goto error;
		}

		curr = mapping_add(mappings, nfa->opt->alloc, dfacount++, set);
		if (curr == NULL) {
			/* TODO: free mappings, set */
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

		/*
		 * The closure of a set is equivalent to the union of closures of
		 * each item. Here we iteratively build up sclosures[] in-situ
		 * to avoid needing to create a state set to store the union.
		 */
		{
			struct state_iter it;
			fsm_state_t s;

			for (state_set_reset(curr->closure, &it); state_set_next(&it, &s); ) {
				/* TODO: could inline this, and destructively hijack state sets from the source NFA */
				if (!symbol_closure_without_epsilons(nfa, s, sclosures)) {
					/* TODO: free mappings, sclosures, stack */
					goto error;
				}
			}
		}

		for (i = 0; i <= FSM_SIGMA_MAX; i++) {
			struct mapping *m;

			if (sclosures[i] == NULL) {
				continue;
			}

			/*
			 * The set of NFA states sclosures[i] represents a single DFA state.
			 * We use the mappings as a de-duplication mechanism, keyed by this
			 * set of NFA states.
			 */

			/* Use an existing mapping if present, otherwise add a new one */
			m = mapping_find(mappings, sclosures[i]);
			if (m != NULL) {
				/* we already have this closure, free this instance */
				state_set_free(sclosures[i]);

				assert(m->dfastate < dfacount);
			} else {
				m = mapping_add(mappings, nfa->opt->alloc, dfacount++, sclosures[i]);
				if (m == NULL) {
					/* TODO: free mappings, sclosures, stack */
					goto error;
				}

				/* ownership belongs to the mapping now, so don't free sclosures[i] */

				if (!stack_push(&stack, nfa->opt->alloc, m)) {
					/* TODO: free mappings, sclosures, stack */
					goto error;
				}
			}

			if (!edge_set_add(&curr->edges, nfa->opt->alloc, i, m->dfastate)) {
				/* TODO: free mappings, sclosures, stack */
				goto error;
			}
		}

		/* all elements in sclosures[] have been freed or moved to their
		 * respective mapping, so there's nothing to free here */
	} while (curr = stack_pop(&stack, nfa->opt->alloc), curr != NULL);

	{
		struct mapping_hashset_iter it;
		struct mapping *m;
		struct fsm *dfa;

		dfa = fsm_new(nfa->opt);
		if (dfa == NULL) {
			goto error;
		}

		if (!fsm_addstate_bulk(dfa, dfacount)) {
			/* TODO: free stuff */
			goto error;
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

			if (!state_set_has(nfa, m->closure, fsm_isend)) {
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

