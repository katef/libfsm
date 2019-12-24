/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/hashset.h>
#include <adt/mappinghashset.h>
#include <adt/statehashset.h>
#include <adt/stateset.h>
#include <adt/statearray.h>
#include <adt/edgeset.h>

#include "internal.h"

enum nfa_transform_op {
	NFA_XFORM_DETERMINISE = 1,
	NFA_XFORM_GLUSHKOVISE
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
	fsm_state_t dfastate;

	/* boolean: are the outgoing edges for dfastate all created? */
	unsigned int done:1;
};

struct fsm_determinise_cache {
	struct mapping_hashset *mappings;
};

static void
clear_mappings(const struct fsm *fsm, struct mapping_hashset *mappings)
{
	struct mapping_hashset_iter it;
	struct mapping *m;

	for (m = mapping_hashset_first(mappings, &it); m != NULL; m = mapping_hashset_next(&it)) {
		state_set_free(m->closure);
		f_free(fsm->opt->alloc, m);
	}

	mapping_hashset_clear(mappings);
}

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

	return state_set_hash(m->closure);
}

/*
 * Find the DFA state associated with a given epsilon closure of NFA states.
 * A new DFA state is created if none exists.
 *
 * The association of DFA states to epsilon closures in the NFA is stored in
 * the mapping set for future reference.
 */
static struct mapping *
addtomappings(struct mapping_hashset *mappings, struct fsm *dfa, struct state_set *closure)
{
	struct mapping *m, search;

	/*
	 * XXX: here we're using hashset_contains() as a key/value lookup,
	 * by only hashing part of struct mapping, coming up with the same
	 * hash for &search and for the stored item, and then retrieving
	 * the other fields in struct mapping.
	 *
	 * Instead it'd be clearer to provide a key/value style API.
	 */

	/* Use an existing mapping if present */
	search.closure = closure;
	m = mapping_hashset_find(mappings, &search);
	if (m != NULL) {
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
	if (!fsm_addstate(dfa, &m->dfastate)) {
		f_free(dfa->opt->alloc, m);
		return NULL;
	}

	m->done = 0;

	if (!mapping_hashset_add(mappings, m)) {
		f_free(dfa->opt->alloc, m);
		return NULL;
	}

	return m;
}

/*
 * Return the DFA state associated with the closure of a given NFA state.
 * Create the DFA state if neccessary.
 */
static int
state_closure(struct mapping_hashset *mappings, struct fsm *dfa,
	const struct fsm *nfa, fsm_state_t nfastate,
	int includeself,
	fsm_state_t *dfastate)
{
	struct mapping *m;
	struct state_set *ec;

	assert(dfa != NULL);
	assert(nfa != NULL);
	assert(nfastate < nfa->statecount);
	assert(mappings != NULL);
	assert(dfastate != NULL);

	ec = NULL;

	if (epsilon_closure(nfa, nfastate, &ec) == NULL) {
		state_set_free(ec);
		return 0;
	}

	if (!includeself) {
		state_set_remove(&ec, nfastate);
	}

	if (state_set_count(ec) == 0) {
		state_set_free(ec);
		return 0;
	}

	m = addtomappings(mappings, dfa, ec);
	if (m == NULL) {
		return 0;
	}

	*dfastate = m->dfastate;
	return 1;
}

/*
 * Return the DFA state associated with the closure of a set of given NFA
 * states. Create the DFA state if neccessary.
 */
static int
set_closure(struct mapping_hashset *mappings, struct fsm *dfa,
	const struct fsm *nfa, struct state_array *set,
	fsm_state_t *dfastate)
{
	struct state_set *ec;
	struct mapping *m;
	size_t i;

	assert(set != NULL);
	assert(set->states != NULL);
	assert(mappings != NULL);
	assert(dfastate != NULL);

	ec = NULL;

	for (i = 0; i < set->len; i++) {
		if (epsilon_closure(nfa, set->states[i], &ec) == NULL) {
			state_set_free(ec);
			return 0;
		}
	}

	m = addtomappings(mappings, dfa, ec);
	if (m == NULL) {
		state_set_free(ec);
		return 0;
	}

	/* TODO: test ec */

	*dfastate = m->dfastate;
	return 1;
}

struct mapping_hashset_iter_save {
	struct mapping_hashset_iter iter;
	int saved;
};

/*
 * Return an arbitary mapping which isn't marked "done" yet.
 */
static struct mapping *
nextnotdone(struct mapping_hashset *mappings, struct mapping_hashset_iter_save *sv)
{
	struct mapping_hashset_iter it;
	struct mapping *m;
	int do_rescan = sv->saved;

	assert(sv != NULL);

	if (sv->saved) {
		it = sv->iter;
		m = mapping_hashset_next(&it);
	} else {
		m = mapping_hashset_first(mappings, &it);
	}

rescan:

	for (; m != NULL; m = mapping_hashset_next(&it)) {
		if (!m->done) {
			sv->saved = 1;
			sv->iter = it;
			return m;
		}
	}

	if (do_rescan) {
		m = mapping_hashset_first(mappings, &it);
		do_rescan = 0;
		goto rescan;
	}

	return NULL;
}

static void
carryend(const struct fsm *src_fsm, const struct state_set *src_set,
	struct fsm *dst_fsm, fsm_state_t dst_state)
{
	struct state_iter it;
	fsm_state_t s;

	/* src_set is >= 1 because a closure includes the source state */
	assert(src_fsm != NULL);
	assert(src_set != NULL && state_set_count(src_set) >= 1);
	assert(dst_fsm != NULL);
	assert(dst_state < dst_fsm->statecount);

	for (state_set_reset((void *) src_set, &it); state_set_next(&it, &s); ) {
		if (fsm_isend(src_fsm, s)) {
			fsm_setend(dst_fsm, dst_state, 1);
			break;
		}
	}
}

/*
 * Builds transitions of the glushkov NFA
 *
 * Makes a single pass through each of the nfa states that the glushkov nfa
 * state corresponds to and constructs an edge set for each symbol out of the
 * nfa states.
 *
 * It does this in one pass by maintaining an array of destination states where
 * the index of the array is the transition symbol.
 */
static int
glushkov_buildtransitions(const struct fsm *fsm, struct fsm *dfa,
	struct mapping_hashset *mappings, struct mapping *curr)
{
	struct state_iter it;
	int sym, sym_min, sym_max;
	fsm_state_t s;
	int ret = 0;

	struct state_array outedges[FSM_SIGMA_COUNT];
	struct state_hashset *outsets[FSM_SIGMA_COUNT];

	assert(fsm != NULL);
	assert(dfa != NULL);
	assert(curr != NULL);
	assert(curr->closure != NULL);

	sym_min = FSM_SIGMA_MAX;
	sym_max = -1;

	memset(outedges, 0, sizeof outedges);
	memset(outsets, 0, sizeof outsets);

	for (state_set_reset(curr->closure, &it); state_set_next(&it, &s); ) {
		struct fsm_edge *e;
		struct edge_iter jt;

		for (e = edge_set_first(fsm->states[s].edges, &jt); e != NULL; e = edge_set_next(&jt)) {
			struct state_iter kt;
			fsm_state_t st;

			sym = e->symbol;

			assert(sym >= 0);
			assert(sym <= UCHAR_MAX);

			if (sym < sym_min) { sym_min = sym; }
			if (sym > sym_max) { sym_max = sym; }

			/* minor optimization: avoid creating a hash set unless
			 * a symbol will have more than one state.
			 *
			 * This might be something we should do at the end when
			 * adding the edges to the new Glushkov NFA.  Then we
			 * could allocate a single hashset and clear it after
			 * every edge symbol.
			 */
			if (outsets[sym] == NULL && outedges[sym].len + state_set_count(e->sl) > 1) {
				size_t i;

				outsets[sym] = state_hashset_create(fsm->opt->alloc);
				if (outsets[sym] == NULL) {
					goto finish;
				}

				/* add any existing states */
				for (i = 0; i < outedges[sym].len; i++) {
					if (!state_hashset_add(outsets[sym], outedges[sym].states[i])) {
						goto finish;
					}
				}
			}

			/* iterate over states, add to state list if they're not already in the set */
			for (state_set_reset(e->sl, &kt); state_set_next(&kt, &st); ) {
				fsm_state_t st_cl;

				if (!state_closure(mappings, dfa, fsm, st, 1, &st_cl)) {
					goto finish;
				}

				if (outsets[sym] == NULL || !state_hashset_contains(outsets[sym], st_cl)) {
					if (!state_array_add(&outedges[sym], st_cl)) {
						goto finish;
					}
				}
			}
		}
	}

	/* double check that the edge symbols are still within 0..UCHAR_MAX */
	if (sym_min < 0 || sym_max >= FSM_SIGMA_MAX) {
		goto finish;
	}

	for (sym = sym_min; sym <= sym_max; sym++) {
		if (outedges[sym].len > 0) {
			if (!fsm_addedge_bulk(dfa, curr->dfastate, outedges[sym].states, outedges[sym].len, sym)) {
				goto finish;
			}
		}
	}

	ret = 1;

finish:

	for (sym = 0; sym < FSM_SIGMA_MAX; sym++) {
		if (outsets[sym] != NULL) {
			state_hashset_free(outsets[sym]);
		}

		if (outedges[sym].states != NULL) {
			free(outedges[sym].states);
		}
	}

	return ret;
}

/*
 * Build transitions from current dfa state to new dfa states.
 *
 * Makes a single pass through each of the nfa states that this dfa state
 * corresponds to and constructs an edge set for each symbol out of the nfa
 * states.
 *
 * It does this in one pass by maintaining an array of destination states where
 * the index of the array is the transition symbol.
 */
static int
dfa_buildtransitions(const struct fsm *fsm, struct fsm *dfa,
	struct mapping_hashset *mappings, struct mapping *curr)
{
	struct state_iter it;
	int sym, sym_min, sym_max;
	fsm_state_t s;
	int ret = 0;

	struct state_array outedges[FSM_SIGMA_COUNT];
	struct state_hashset *outsets[FSM_SIGMA_COUNT];

	assert(fsm != NULL);
	assert(dfa != NULL);
	assert(curr != NULL);
	assert(curr->closure != NULL);

	sym_min = UCHAR_MAX+1;
	sym_max = -1;

	memset(outedges, 0, sizeof outedges);
	memset(outsets, 0, sizeof outsets);

	for (state_set_reset(curr->closure, &it); state_set_next(&it, &s); ) {
		struct fsm_edge *e;
		struct edge_iter jt;

		for (e = edge_set_first(fsm->states[s].edges, &jt); e != NULL; e = edge_set_next(&jt)) {
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
				struct state_iter kt;
				fsm_state_t st;

				/* wait for a second edge to make a hash set */
				if (outsets[sym] == NULL) {
					size_t i;

					outsets[sym] = state_hashset_create(fsm->opt->alloc);
					if (outsets[sym] == NULL) {
						goto finish;
					}

					assert(outedges[sym].states != NULL);
					assert(outedges[sym].len > 0);

					/* add states from first edge */
					for (i = 0; i < outedges[sym].len; i++) {
						if (!state_hashset_add(outsets[sym], outedges[sym].states[i])) {
							goto finish;
						}
					}
				}

				/* iterate over states, add to state list if they're not already in the set */
				for (state_set_reset(e->sl, &kt); state_set_next(&kt, &st); ) {
					if (!state_hashset_contains(outsets[sym], st)) {
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
		fsm_state_t new;

		if (outedges[sym].len == 0) {
			continue;
		}

		if (!set_closure(mappings, dfa, fsm, &outedges[sym], &new)) {
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
			state_hashset_free(outsets[sym]);
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
nfa_transform(struct fsm *nfa,
	struct fsm_determinise_cache *dcache, enum nfa_transform_op op)
{
	static const struct mapping_hashset_iter_save sv_init;

	struct mapping *curr;
	struct mapping_hashset *mappings;
	struct mapping_hashset_iter it;
	struct fsm *dfa;

	struct mapping_hashset_iter_save sv;

	assert(nfa != NULL);
	assert(nfa->opt != NULL);
	assert(dcache != NULL);

	dfa = fsm_new(nfa->opt);
	if (dfa == NULL) {
		return NULL;
	}

	if (nfa->endcount == 0) {
		fsm_state_t start;

		if (!fsm_addstate(dfa, &start)) {
			fsm_free(dfa);
			return NULL;
		}

		fsm_setstart(dfa, start);

		return dfa;
	}

	if (dcache->mappings == NULL) {
		dcache->mappings = mapping_hashset_create(nfa->opt->alloc, hash_mapping, cmp_mapping);
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
		fsm_state_t nfastart;
		fsm_state_t dfastart;
		int includeself = 1;

		if (!fsm_getstart(nfa, &nfastart)) {
			errno = EINVAL;
			return NULL;
		}

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

		if (!state_closure(mappings, dfa, nfa, nfastart, includeself, &dfastart)) {
			/* TODO: error */
			goto error;
		}

		fsm_setstart(dfa, dfastart);
	}

	/*
	 * While there are still states remaining to be "done", process each.
	 */
	sv = sv_init;
	for (curr = mapping_hashset_first(mappings, &it);
		(curr = nextnotdone(mappings, &sv)) != NULL;
		curr->done = 1)
	{
		if (op == NFA_XFORM_DETERMINISE) {
			if (!dfa_buildtransitions(nfa, dfa, mappings, curr)) {
				goto error;
			}
		} else if (op == NFA_XFORM_GLUSHKOVISE) {
			if (!glushkov_buildtransitions(nfa, dfa, mappings, curr)) {
				goto error;
			}
		}

		/*
		 * The current DFA state is an end state if any of its associated NFA
		 * states are end states.
		 */
		carryend(nfa, curr->closure, dfa, curr->dfastate);
		if (!fsm_isend(dfa, curr->dfastate)) {
			continue;
		}

		/*
		 * Carry through opaque values, if present. This isn't anything to do
		 * with the DFA conversion; it's meaningful only to the caller.
		 *
		 * The closure may contain non-end states, but at least one state is
		 * known to have been an end state.
		 */
		fsm_carryopaque(nfa, curr->closure, dfa, curr->dfastate);
	}

	clear_mappings(dfa, mappings);

	/* TODO: can assert a whole bunch of things about the dfa, here */
	assert(op != NFA_XFORM_DETERMINISE || fsm_all(dfa, fsm_isdfa));
	assert(op != NFA_XFORM_GLUSHKOVISE || fsm_count(dfa, fsm_hasepsilons) == 0);

	return dfa;

error:

	clear_mappings(dfa, mappings);
	fsm_free(dfa);

	return NULL;
}

static int
nfa_transform_cache(struct fsm *fsm,
	struct fsm_determinise_cache **dcache, enum nfa_transform_op op)
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

		(*dcache)->mappings = mapping_hashset_create(fsm->opt->alloc, hash_mapping, cmp_mapping);
		if ((*dcache)->mappings == NULL) {
			f_free(fsm->opt->alloc, *dcache);
			return 0;
		}
	}

	dfa = nfa_transform(fsm, *dcache, op);
	if (dfa == NULL) {
		return 0;
	}

	fsm_move(fsm, dfa);

	return 1;
}

int
fsm_determinise_cache(struct fsm *fsm,
	struct fsm_determinise_cache **dcache)
{
	return nfa_transform_cache(fsm, dcache, NFA_XFORM_DETERMINISE);
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
		mapping_hashset_free(dcache->mappings);
	}

	f_free(fsm->opt->alloc, dcache);
}

#if 0
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
#endif

#if 0
int
fsm_glushkovise(struct fsm *fsm)
{
	struct fsm_determinise_cache *dcache;
	int r;

	dcache = NULL;

	r = nfa_transform_cache(fsm, &dcache, NFA_XFORM_GLUSHKOVISE);

	fsm_determinise_freecache(fsm, dcache);

	return r;
}
#endif

