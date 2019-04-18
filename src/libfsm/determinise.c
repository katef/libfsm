/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/hashset.h>
#include <adt/mappingset.h>
#include <adt/stateset.h>
#include <adt/statearray.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include "internal.h"

/*
 * A set of states in an NFA.
 *
 * These have labels naming a transition which was followed to reach the state.
 * This is used for finding which states are reachable by a given label, given
 * a set of several states.
 */
struct trans {
	const struct fsm_state *state;
	char c;
};

/*
 * This maps a DFA state onto its associated NFA epsilon closure, such that an
 * existing DFA state may be found given any particular set of NFA states
 * forming an epsilon closure.
 *
 * These mappings are kept in a list; order does not matter.
 */

struct mapping {
	/* The set of NFA states forming the epsilon closure for this DFA state */
	struct state_set *closure;

	/* The DFA state associated with this epsilon closure of NFA states */
	struct fsm_state *dfastate;

	/* boolean: are the outgoing edges for dfastate all created? */
	unsigned int done:1;
};

struct fsm_determinise_cache {
	struct mapping_set *mappings;
};

static void
clear_mappings(const struct fsm *fsm, struct mapping_set *mappings)
{
	struct mapping_iter it;
	struct mapping *m;

	for (m = mapping_set_first(mappings, &it); m != NULL; m = mapping_set_next(&it)) {
		state_set_free(m->closure);
		f_free(fsm->opt->alloc, m);
	}

	mapping_set_clear(mappings);
}

static int
cmp_mapping(const void *a, const void *b)
{
	const struct mapping *ma, *mb;

	assert(a != NULL);
	assert(b != NULL);

	ma = a;
	mb = b;

	return state_set_cmp(ma->closure, mb->closure);
}

static unsigned long
hash_mapping(const void *a)
{
	const struct mapping *m = a;
	const struct fsm_state **states = state_set_array(m->closure);
	size_t nstates = state_set_count(m->closure);

	return hashrec(states, nstates * sizeof states[0]);
}

/*
 * Find the DFA state associated with a given epsilon closure of NFA states.
 * A new DFA state is created if none exists.
 *
 * The association of DFA states to epsilon closures in the NFA is stored in
 * the mapping set for future reference.
 */
static struct mapping *
addtomappings(struct mapping_set *mappings, struct fsm *dfa, struct state_set *closure)
{
	struct mapping *m, search;

	/* Use an existing mapping if present */
	search.closure = closure;
	if ((m = mapping_set_contains(mappings, &search))) {
		state_set_free(closure);
		return m;
	}

	/* else add new DFA state */
	m = f_malloc(dfa->opt->alloc, sizeof *m);
	if (m == NULL) {
		return NULL;
	}

	assert(closure != NULL);
	m->closure  = closure;
	m->dfastate = fsm_addstate(dfa);
	if (m->dfastate == NULL) {
		f_free(dfa->opt->alloc, m);
		return NULL;
	}

	m->done = 0;

	if (!mapping_set_add(mappings, m)) {
		f_free(dfa->opt->alloc, m);
		return NULL;
	}

	return m;
}

/*
 * Return the DFA state associated with the closure of a given NFA state.
 * Create the DFA state if neccessary.
 */
static struct fsm_state *
state_closure(struct mapping_set *mappings, struct fsm *dfa, const struct fsm_state *nfastate,
	int includeself)
{
	struct mapping *m;
	struct state_set *ec;

	assert(dfa != NULL);
	assert(nfastate != NULL);
	assert(mappings != NULL);

	ec = state_set_create(dfa->opt->alloc);
	if (ec == NULL) {
		return NULL;
	}

	if (epsilon_closure(nfastate, ec) == NULL) {
		state_set_free(ec);
		return NULL;
	}

	if (!includeself) {
		state_set_remove(ec, (void *) nfastate);
	}

	if (state_set_count(ec) == 0) {
		state_set_free(ec);
		return NULL;
	}

	m = addtomappings(mappings, dfa, ec);
	if (m == NULL) {
		return NULL;
	}

	return m->dfastate;
}

/*
 * Return the DFA state associated with the closure of a set of given NFA
 * states. Create the DFA state if neccessary.
 */
static struct fsm_state *
set_closure(struct mapping_set *mappings, struct fsm *dfa, struct state_set *set)
{
	struct state_iter it;
	struct state_set *ec;
	struct mapping *m;
	struct fsm_state *s;

	assert(set != NULL);
	assert(mappings != NULL);

	ec = state_set_create(dfa->opt->alloc);
	if (ec == NULL) {
		return NULL;
	}

	for (s = state_set_first(set, &it); s != NULL; s = state_set_next(&it)) {
		if (epsilon_closure(s, ec) == NULL) {
			state_set_free(ec);
			return NULL;
		}
	}

	m = addtomappings(mappings, dfa, ec);
	if (m == NULL) {
		state_set_free(ec);
		return NULL;
	}

	/* TODO: test ec */

	return m->dfastate;
}

struct mapping_iter_save {
	struct mapping_iter iter;
	int saved;
};

/*
 * Return an arbitary mapping which isn't marked "done" yet.
 */
static struct mapping *
nextnotdone(struct mapping_set *mappings, struct mapping_iter_save *sv)
{
	struct mapping_iter it;
	struct mapping *m;
	int do_rescan = sv->saved;

	assert(sv != NULL);

	if (sv->saved) {
		it = sv->iter;
		m = mapping_set_next(&it);
	} else {
		m = mapping_set_first(mappings, &it);
	}

rescan:
	for (; m != NULL; m = mapping_set_next(&it)) {
		if (!m->done) {
			sv->saved = 1;
			sv->iter = it;
			return m;
		}
	}

	if (do_rescan) {
		m = mapping_set_first(mappings, &it);
		do_rescan = 0;
		goto rescan;
	}

	return NULL;
}

static void
carryend(struct state_set *set, struct fsm *fsm, struct fsm_state *state)
{
	struct state_iter it;
	struct fsm_state *s;

	assert(set != NULL); /* TODO: right? */
	assert(fsm != NULL);
	assert(state != NULL);

	for (s = state_set_first(set, &it); s != NULL; s = state_set_next(&it)) {
		if (fsm_isend(fsm, s)) {
			fsm_setend(fsm, state, 1);
		}
	}
}

static int
cmp_single_state(const void *a, const void *b)
{
	const struct fsm_state *ea = a, *eb = b;
	return (ea > eb) - (ea < eb);
}

static unsigned long
hash_single_state(const void *a)
{
	const struct fsm_state *st = a;
	return hashrec(st, sizeof *st);
}

/* builds transitions from current dfa state to new dfa states
 *
 * Makes a single pass through each of the nfa states that this dfa state
 * corresponds to and constructs an edge set for each symbol out of the nfa
 * states.
 *
 * It does this in one pass by maintaining an array of destination states where
 * the index of the array is the transition symbol.
 */
static int
buildtransitions(const struct fsm *fsm, struct fsm *dfa, struct mapping_set *mappings, struct mapping *curr)
{
	struct fsm_state *s;
	struct state_iter it;
	int sym, sym_min, sym_max;
	int ret = 0;

	struct state_array outedges[FSM_SIGMA_COUNT];
	struct hashset *outsets[FSM_SIGMA_COUNT];

	assert(fsm != NULL);
	assert(dfa != NULL);
	assert(curr != NULL);
	assert(curr->closure != NULL);

	sym_min = UCHAR_MAX+1;
	sym_max = -1;

	memset(outedges, 0, sizeof outedges);
	memset(outsets, 0, sizeof outsets);

	for (s = state_set_first(curr->closure, &it); s != NULL; s = state_set_next(&it)) {
		struct fsm_edge *e;
		struct edge_iter jt;

		for (e = edge_set_first(s->edges, &jt); e != NULL; e = edge_set_next(&jt)) {
			sym = e->symbol;

			assert(sym >= 0);
			assert(sym <= UCHAR_MAX);

			if (sym < sym_min) { sym_min = sym; }
			if (sym > sym_max) { sym_max = sym; }

			/* first edge, we just copy the states */
			if (outedges[sym].states == NULL) {
				if (!state_array_copy_set(&outedges[sym], e->sl)) {
					goto finish;
				}
			} else {
				struct fsm_state *st;
				struct state_iter kt;

				/* wait for a second edge to make a hash set */
				if (outsets[sym] == NULL) {
					size_t i;

					outsets[sym] = hashset_create(fsm->opt->alloc, hash_single_state, cmp_single_state);
					if (outsets[sym] == NULL) {
						goto finish;
					}

					assert(outedges[sym].states != NULL);
					assert(outedges[sym].len > 0);

					/* add states from first edge */
					for (i=0; i < outedges[sym].len; i++) {
						if (!hashset_add(outsets[sym], outedges[sym].states[i])) {
							goto finish;
						}
					}
				}

				/* iterate over states, add to state list if they're not already in the set */
				for (st = state_set_first(e->sl, &kt); st != NULL; st = state_set_next(&kt)) {
					if (!hashset_contains(outsets[sym], st)) {
						if (!state_array_add(&outedges[sym], st)) {
							goto finish;
						}
					}
				}
			}
		}
	}

	/* double check that the edge symbols are still within 0..UCHAR_MAX */
	if (sym_min < 0 || sym_max > UCHAR_MAX) {
		goto finish;
	}

	for (sym = sym_min; sym <= sym_max; sym++) {
		struct state_set *reachable;
		struct fsm_state *new;

		if (outedges[sym].len == 0) {
			continue;
		}

		reachable = state_set_create_from_array(fsm->opt->alloc, outedges[sym].states, outedges[sym].len);
		if (reachable == NULL) {
			goto finish;
		}

		/* reachable now owns the outedge[sym].states array, so reset to
		 * zero to avoid a free() in finish */
		memset(&outedges[sym], 0, sizeof outedges[sym]);

		new = set_closure(mappings, dfa, reachable);
		state_set_free(reachable);
		if (new == NULL) {
			goto finish;
		}

		if (!fsm_addedge_literal(dfa, curr->dfastate, new, sym)) {
			goto finish;
		}
	}

	ret = 1;

finish:
	for (sym = 0; sym < UCHAR_MAX+1; sym++) {
		if (outsets[sym] != NULL) {
			hashset_free(outsets[sym]);
		}

		if (outedges[sym].states != NULL) {
			free(outedges[sym].states);
		}
	}

	return ret;
}


/*
 * Convert an NFA to a DFA. This is the guts of conversion; it is an
 * implementation of the well-known multiple-states method. This produces a DFA
 * which simulates the given NFA by collating all reachable NFA states
 * simultaneously. The basic principle behind this is a closure on epsilon
 * transitions, which produces the set of all reachable NFA states without
 * consuming any input. This set of NFA states becomes a single state in the
 * newly-created DFA.
 *
 * For a more in-depth discussion, see (for example) chapter 2 of Appel's
 * "Modern compiler implementation", which has a detailed description of this
 * process.
 *
 * As all DFA are NFA; for a DFA this has no semantic effect (other than
 * renumbering states as a side-effect of constructing the new FSM).
 */
static struct fsm *
determinise(struct fsm *nfa,
	struct fsm_determinise_cache *dcache)
{
	static const struct mapping_iter_save sv_init;

	struct mapping *curr;
	struct mapping_set *mappings;
	struct mapping_iter it;
	struct fsm *dfa;

	struct mapping_iter_save sv;

	assert(nfa != NULL);
	assert(nfa->opt != NULL);
	assert(dcache != NULL);

	dfa = fsm_new(nfa->opt);
	if (dfa == NULL) {
		return NULL;
	}

	if (nfa->endcount == 0) {
		dfa->start = fsm_addstate(dfa);
		if (dfa->start == NULL) {
			fsm_free(dfa);
			return NULL;
		}

		return dfa;
	}

	if (dcache->mappings == NULL) {
		dcache->mappings = mapping_set_create(nfa->opt->alloc, hash_mapping, cmp_mapping);
		if (dcache->mappings == NULL) {
			fsm_free(dfa);
			return NULL;
		}
	}
	mappings = dcache->mappings;

	/*
	 * The epsilon closure of the NFA's start state is the DFA's start state.
	 * This is not yet "done"; it starts off the loop below.
	 */
	{
		const struct fsm_state *nfastart;
		struct fsm_state *dfastart;
		int includeself = 1;

		nfastart = fsm_getstart(nfa);

		/*
		 * As a special case for Brzozowski's algorithm, fsm_determinise() is
		 * expected to produce a minimal DFA for its invocation after the second
		 * reversal. Since we do not provide multiple start states, fsm_reverse()
		 * may introduce a new start state which transitions to several states.
		 * This is the situation we detect here.
		 *
		 * This fabricated start state is then excluded from its epsilon closure,
		 * so that the closures for its destination states are found equivalent,
		 * because they also do not include the start state.
		 *
		 * If you pass an equivalent NFA where this is not the case (for example,
		 * with the start state containing an epsilon edge to itself), we regard
		 * this as any other DFA, and minimal output is not guaranteed.
		 */
		if (!fsm_isend(nfa, nfastart)
			&& fsm_epsilonsonly(nfa, nfastart) && !fsm_hasincoming(nfa, nfastart))
		{
			includeself = 0;
		}

		dfastart = state_closure(mappings, dfa, nfastart, includeself);
		if (dfastart == NULL) {
			/* TODO: error */
			goto error;
		}

		fsm_setstart(dfa, dfastart);
	}

	/*
	 * While there are still DFA states remaining to be "done", process each.
	 */
	sv = sv_init;
	for (curr = mapping_set_first(mappings, &it); (curr = nextnotdone(mappings, &sv)) != NULL; curr->done = 1) {
		if (!buildtransitions(nfa, dfa, mappings, curr)) {
			goto error;
		}

		/*
		 * The current DFA state is an end state if any of its associated NFA
		 * states are end states.
		 */
		carryend(curr->closure, dfa, curr->dfastate);

		/*
		 * Carry through opaque values, if present. This isn't anything to do
		 * with the DFA conversion; it's meaningful only to the caller.
		 */
		fsm_carryopaque(dfa, curr->closure, dfa, curr->dfastate);
	}

	clear_mappings(dfa, mappings);

	/* TODO: can assert a whole bunch of things about the dfa, here */
	assert(fsm_all(dfa, fsm_isdfa));

	return dfa;

error:

	clear_mappings(dfa, mappings);
	fsm_free(dfa);

	return NULL;
}

int
fsm_determinise_cache(struct fsm *fsm,
	struct fsm_determinise_cache **dcache)
{
	struct fsm *dfa;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);
	assert(dcache != NULL);

	if (*dcache == NULL) {
		*dcache = f_malloc(fsm->opt->alloc, sizeof **dcache);
		if (*dcache == NULL) {
			return 0;
		}

		(*dcache)->mappings = mapping_set_create(fsm->opt->alloc, hash_mapping, cmp_mapping);
		if ((*dcache)->mappings == NULL) {
			f_free(fsm->opt->alloc, *dcache);
			return 0;
		}
	}

	dfa = determinise(fsm, *dcache);
	if (dfa == NULL) {
		return 0;
	}

	fsm_move(fsm, dfa);

	return 1;
}

void
fsm_determinise_freecache(struct fsm *fsm, struct fsm_determinise_cache *dcache)
{
	(void) fsm;

	if (dcache == NULL) {
		return;
	}

	clear_mappings(fsm, dcache->mappings);

	if (dcache->mappings != NULL) {
		mapping_set_free(dcache->mappings);
	}

	f_free(fsm->opt->alloc, dcache);
}

int
fsm_determinise(struct fsm *fsm)
{
	struct fsm_determinise_cache *dcache;
	int r;

	dcache = NULL;

	r = fsm_determinise_cache(fsm, &dcache);

	fsm_determinise_freecache(fsm, dcache);

	return r;
}

