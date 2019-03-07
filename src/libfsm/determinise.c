/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <adt/set.h>

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
	struct set *closure;

	/* The DFA state associated with this epsilon closure of NFA states */
	struct fsm_state *dfastate;

	/* boolean: are the outgoing edges for dfastate all created? */
	unsigned int done:1;
};

struct fsm_determinise_cache {
	struct set *mappings;
	struct set *trans;
};

static void
clear_trans(const struct fsm *fsm, struct set *trans)
{
	struct set_iter it;
	struct trans *t;

	for (t = set_first(trans, &it); t != NULL; t = set_next(&it)) {
		f_free(fsm, t);
	}

	set_clear(trans);
}

static void
clear_mappings(const struct fsm *fsm, struct set *mappings)
{
	struct set_iter it;
	struct mapping *m;

	for (m = set_first(mappings, &it); m != NULL; m = set_next(&it)) {
		set_free(m->closure);
		f_free(fsm, m);
	}

	set_clear(mappings);
}

static int
cmp_trans(const void *a, const void *b)
{
	const struct trans *ta, *tb;

	assert(a != NULL);
	assert(b != NULL);

	ta = a;
	tb = b;

	return (ta->c > tb->c) - (ta->c < tb->c);
}

static int
cmp_mapping(const void *a, const void *b)
{
	const struct mapping *ma, *mb;

	assert(a != NULL);
	assert(b != NULL);

	ma = a;
	mb = b;

	return set_cmp(ma->closure, mb->closure);
}

/*
 * Find the DFA state associated with a given epsilon closure of NFA states.
 * A new DFA state is created if none exists.
 *
 * The association of DFA states to epsilon closures in the NFA is stored in
 * the mapping set for future reference.
 */
static struct mapping *
addtomappings(struct set *mappings, struct fsm *dfa, struct set *closure)
{
	struct mapping *m, search;

	/* Use an existing mapping if present */
	search.closure = closure;
	if ((m = set_contains(mappings, &search))) {
		set_free(closure);
		return m;
	}

	/* else add new DFA state */
	m = f_malloc(dfa, sizeof *m);
	if (m == NULL) {
		return NULL;
	}

	assert(closure != NULL);
	m->closure  = closure;
	m->dfastate = fsm_addstate(dfa);
	if (m->dfastate == NULL) {
		f_free(dfa, m);
		return NULL;
	}

	m->done = 0;

	if (!set_add(&mappings, m)) {
		f_free(dfa, m);
		return NULL;
	}

	return m;
}

/*
 * Return a list of each state in the epsilon closure of the given state.
 * These are all the states reachable through epsilon transitions (that is,
 * without consuming any input by traversing a labelled edge), including the
 * given state itself.
 *
 * Intermediate states consisting entirely of epsilon transitions are
 * considered part of the closure.
 *
 * This is an internal routine for convenience of recursion; the
 * state_closure() and set_closure() interfaces ought to be called, instead.
 *
 * Returns closure on success, NULL on error.
 */
static struct set **
epsilon_closure(const struct fsm_state *state, struct set **closure)
{
	struct fsm_state *s;
	struct fsm_edge *e;
	struct set_iter it;

	assert(state != NULL);
	assert(closure != NULL);

	/* Find if the given state is already in the closure */
	if (set_contains(*closure, state)) {
		return closure;
	}

	if (!set_add(closure, (void *) state)) {
		return NULL;
	}

	/* Follow each epsilon transition */
	if ((e = fsm_hasedge(state, FSM_EDGE_EPSILON)) != NULL) {
		for (s = set_first(e->sl, &it); s != NULL; s = set_next(&it)) {
			assert(s != NULL);

			if (epsilon_closure(s, closure) == NULL) {
				return NULL;
			}
		}
	}

	return closure;
}

/*
 * Return the DFA state associated with the closure of a given NFA state.
 * Create the DFA state if neccessary.
 */
static struct fsm_state *
state_closure(struct set *mappings, struct fsm *dfa, const struct fsm_state *nfastate,
	int includeself)
{
	struct mapping *m;
	struct set *ec;

	assert(dfa != NULL);
	assert(nfastate != NULL);
	assert(mappings != NULL);

	ec = NULL;
	if (epsilon_closure(nfastate, &ec) == NULL) {
		set_free(ec);
		return NULL;
	}

	if (!includeself) {
		set_remove(&ec, (void *) nfastate);

		if (set_count(ec) == 0) {
			set_free(ec);
			ec = NULL;
		}
	}

	if (ec == NULL) {
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
set_closure(struct set *mappings, struct fsm *dfa, struct set *set)
{
	struct set_iter it;
	struct set *ec;
	struct mapping *m;
	struct fsm_state *s;

	assert(set != NULL);
	assert(mappings != NULL);

	ec = NULL;
	for (s = set_first(set, &it); s != NULL; s = set_next(&it)) {
		if (epsilon_closure(s, &ec) == NULL) {
			set_free(ec);
			return NULL;
		}
	}

	m = addtomappings(mappings, dfa, ec);
	/* TODO: test ec */

	return m->dfastate;
}

/*
 * Return an arbitary mapping which isn't marked "done" yet.
 */
static struct mapping *
nextnotdone(struct set *mappings)
{
	struct set_iter it;
	struct mapping *m;

	for (m = set_first(mappings, &it); m != NULL; m = set_next(&it)) {
		if (!m->done) {
			return m;
		}
	}

	return NULL;
}

/*
 * List all states within a set which are reachable via non-epsilon
 * transitions (that is, have a label associated with them).
 *
 * TODO: maybe simpler to just return the set, rather than take a double pointer
 */
static int
listnonepsilonstates(const struct fsm *fsm ,struct set *trans, struct set *set)
{
	struct fsm_state *s;
	struct set_iter it;

	assert(set != NULL);
	assert(trans != NULL);

	for (s = set_first(set, &it); s != NULL; s = set_next(&it)) {
		struct fsm_edge *e;
		struct set_iter jt;

		for (e = set_first(s->edges, &jt); e != NULL; e = set_next(&jt)) {
			struct fsm_state *st;
			struct set_iter kt;

			if (e->symbol > UCHAR_MAX) {
				break;
			}

			for (st = set_first(e->sl, &kt); st != NULL; st = set_next(&kt)) {
				struct trans *p, search;

				assert(st != NULL);

				/* Skip transitions we've already got */
				search.c = e->symbol;
				if ((p = set_contains(trans, &search))) {
					continue;
				}

				p = f_malloc(fsm, sizeof *p);
				if (p == NULL) {
					clear_trans(fsm ,trans);
					return 0;
				}

				p->c = e->symbol;
				p->state = st;

				if (!set_add(&trans, p)) {
					f_free(fsm, p);
					clear_trans(fsm, trans);
					return 0;
				}
			}
		}
	}

	return 1;
}

/*
 * Return a list of all states reachable from set via the given transition.
 */
static int
allstatesreachableby(struct set *set, char c, struct set **sl)
{
	struct fsm_state *s;
	struct set_iter it;

	assert(set != NULL);

	for (s = set_first(set, &it); s != NULL; s = set_next(&it)) {
		struct fsm_state *es;
		struct fsm_edge *to;

		if ((to = fsm_hasedge(s, (unsigned char) c)) != NULL) {
			struct set_iter jt;

			for (es = set_first(to->sl, &jt); es != NULL; es = set_next(&jt)) {
				assert(es != NULL);

				if (!set_add(sl, es)) {
					return 0;
				}
			}
		}
	}

	return 1;
}

static void
carryend(struct set *set, struct fsm *fsm, struct fsm_state *state)
{
	struct set_iter it;
	struct fsm_state *s;

	assert(set != NULL); /* TODO: right? */
	assert(fsm != NULL);
	assert(state != NULL);

	for (s = set_first(set, &it); s != NULL; s = set_next(&it)) {
		if (fsm_isend(fsm, s)) {
			fsm_setend(fsm, state, 1);
		}
	}
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
	struct mapping *curr;
	struct set *mappings;
	struct set_iter it;
	struct set *trans;
	struct fsm *dfa;

	assert(nfa != NULL);
	assert(nfa->opt != NULL);
	assert(dcache != NULL);

	dfa = fsm_new(nfa->opt);
	if (dfa == NULL) {
		return NULL;
	}

#ifdef DEBUG_TODFA
	dfa->nfa = nfa;
#endif

	if (nfa->endcount == 0) {
		dfa->start = fsm_addstate(dfa);
		if (dfa->start == NULL) {
			fsm_free(dfa);
			return NULL;
		}

		return dfa;
	}

	if (dcache->mappings == NULL) {
		dcache->mappings = set_create(cmp_mapping);
	}
	mappings = dcache->mappings;
	assert(mappings != NULL);

	if (dcache->trans == NULL) {
		dcache->trans = set_create(cmp_trans);
	}
	trans = dcache->trans;
	assert(trans != NULL);

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
	for (curr = set_first(mappings, &it); (curr = nextnotdone(mappings)) != NULL; curr->done = 1) {
		struct set_iter jt;
		struct trans *t;

		/*
		 * Loop over every unique non-epsilon transition out of curr's epsilon
		 * closure.
		 *
		 * This is a set of labels. Since curr->closure is already a closure
		 * (computed on insertion to mappings), these labels directly reach the
		 * next states in the NFA.
		 */
		/* TODO: document that nes contains only entries with labels set */
		if (!listnonepsilonstates(dfa, trans, curr->closure)) {
			goto error;
		}

		for (t = set_first(trans, &jt); t != NULL; t = set_next(&jt)) {
			struct fsm_state *new;
			struct fsm_edge *e;
			struct set *reachable;

			assert(t->state != NULL);

			reachable = NULL;

			/*
			 * Find the closure of the set of all NFA states which are reachable
			 * through this label, starting from the set of states forming curr's
			 * closure.
			 */
			if (!allstatesreachableby(curr->closure, t->c, &reachable)) {
				set_free(reachable);
				goto error;
			}

			new = set_closure(mappings, dfa, reachable);
			set_free(reachable);
			if (new == NULL) {
				clear_trans(dfa, trans);
				goto error;
			}

			e = fsm_addedge_literal(dfa, curr->dfastate, new, t->c);
			if (e == NULL) {
				clear_trans(dfa, trans);
				goto error;
			}
		}

		clear_trans(dfa, trans);

#ifdef DEBUG_TODFA
		{
			struct set *q;

			for (q = set_first(curr->closure, &jt); q != NULL; q = set_next(&jt)) {
				if (!set_add(&curr->dfastate->nfasl, q)) {
					goto error;
				}
			}
		}
#endif

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
		*dcache = f_malloc(fsm, sizeof **dcache);
		if (*dcache == NULL) {
			return 0;
		}

		(*dcache)->mappings = NULL;
		(*dcache)->trans    = NULL;
	}

	dfa = determinise(fsm, *dcache);
	if (dfa == NULL) {
		return 0;
	}

#ifdef DEBUG_TODFA
	fsm->nfa = fsm_new(fsm->opt);
	if (fsm->nfa == NULL) {
		return 0;
	}

	assert(dfa->nfa == fsm);

	fsm->nfa->sl    = fsm->sl;
	fsm->nfa->tail  = fsm->tail;
	fsm->nfa->start = fsm->start;

	/* for fsm_move's free contents */
	fsm->sl    = NULL;
	fsm->tail  = NULL;
	fsm->start = NULL;
#endif

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
	clear_trans(fsm, dcache->trans);

	if (dcache->mappings != NULL) {
		set_free(dcache->mappings);
	}

	if (dcache->trans != NULL) {
		set_free(dcache->trans);
	}

	f_free(fsm, dcache);
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

