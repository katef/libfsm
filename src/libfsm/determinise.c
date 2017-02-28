/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

/*
 * A set of states in an NFA.
 *
 * These have labels naming a transition which was followed to reach the state.
 * This is used for finding which states are reachable by a given label, given
 * a set of several states.
 */
struct transset {
	const struct fsm_state *state;
	char c;
	struct transset *next;
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

	struct mapping *next;
};

static void
free_transset(struct transset *set)
{
	struct transset *p;
	struct transset *next;

	for (p = set; p != NULL; p = next) {
		next = p->next;
		free(p);
	}
}

static void
free_mappings(struct mapping *m)
{
	struct mapping *p;
	struct mapping *next;

	for (p = m; p != NULL; p = next) {
		next = p->next;
		free(p);
	}
}

/* Find if a transition is in a set */
static int
transin(char c, const struct transset *set)
{
	const struct transset *p;

	for (p = set; p != NULL; p = p->next) {
		assert(p->state != NULL);

		if (p->c == c) {
			return 1;
		}
	}

	return 0;
}

/*
 * Find the DFA state associated with a given epsilon closure of NFA states.
 * A new DFA state is created if none exists.
 *
 * The association of DFA states to epsilon closures in the NFA is stored in
 * the mapping list ml for future reference.
 */
static struct mapping *
addtoml(struct fsm *dfa, struct mapping **ml, struct set *closure)
{
	struct mapping *m;

	/* use existing mapping if present */
	for (m = *ml; m != NULL; m = m->next) {
		if (set_equal(m->closure, closure)) {
			set_free(closure);
			return m;
		}
	}

	/* else add new DFA state */
	m = malloc(sizeof *m);
	if (m == NULL) {
		return NULL;
	}

	assert(closure != NULL);
	m->closure  = closure;
	m->dfastate = fsm_addstate(dfa);
	if (m->dfastate == NULL) {
		free(m);
		return NULL;
	}

	m->done = 0;
	m->next = *ml;
	*ml = m;

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

	set_add(closure, state);

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
state_closure(struct mapping **ml, struct fsm *dfa, const struct fsm_state *nfastate)
{
	struct set *ec;
	struct mapping *m;

	assert(ml != NULL);
	assert(dfa != NULL);
	assert(nfastate != NULL);

	ec = NULL;
	if (epsilon_closure(nfastate, &ec) == NULL) {
		set_free(ec);
		return NULL;
	}

	if (ec == NULL) {
		return NULL;
	}

	m = addtoml(dfa, ml, ec);
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
set_closure(struct mapping **ml, struct fsm *dfa, struct set *set)
{
	struct set_iter it;
	struct set *ec;
	struct mapping *m;
	struct fsm_state *s;

	assert(ml != NULL);
	assert(set != NULL);

	ec = NULL;
	for (s = set_first(set, &it); s != NULL; s = set_next(&it)) {
		if (epsilon_closure(s, &ec) == NULL) {
			set_free(ec);
			return NULL;
		}
	}

	m = addtoml(dfa, ml, ec);
	/* TODO: test ec */

	return m->dfastate;
}

/*
 * Return an arbitary mapping which isn't marked "done" yet.
 */
static struct mapping *
nextnotdone(struct mapping *ml)
{
	struct mapping *m;

	for (m = ml; m != NULL; m = m->next) {
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
listnonepsilonstates(struct transset **l, struct set *set)
{
	struct fsm_state *s;
	struct set_iter it;

	assert(l != NULL);
	assert(set != NULL);

	*l = NULL;
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
				struct transset *p;

				assert(st != NULL);

				/* Skip transitions we've already got */
				if (transin(e->symbol, *l)) {
					continue;
				}

				p = malloc(sizeof *p);
				if (p == NULL) {
					free_transset(*l);
					return 0;
				}

				p->c = e->symbol;
				p->state = st;

				p->next = *l;
				*l = p;
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
	void (*carryopaque)(struct set *, struct fsm *, struct fsm_state *))
{
	struct mapping *curr;
	struct mapping *ml;
	struct fsm *dfa;

	assert(nfa != NULL);

	dfa = fsm_new();
	if (dfa == NULL) {
		return NULL;
	}

#ifdef DEBUG_TODFA
	dfa->nfa = nfa;
#endif

	ml = NULL;

	/*
	 * The epsilon closure of the NFA's start state is the DFA's start state.
	 * This is not yet "done"; it starts off the loop below.
	 */
	{
		struct fsm_state *dfastart;

		dfastart = state_closure(&ml, dfa, fsm_getstart(nfa));
		if (dfastart == NULL) {
			/* TODO: error */
			goto error;
		}

		fsm_setstart(dfa, dfastart);
	}

	/*
	 * While there are still DFA states remaining to be "done", process each.
	 */
	for (curr = ml; (curr = nextnotdone(ml)) != NULL; curr->done = 1) {
		struct transset *s;
		struct transset *nes;

		/*
		 * Loop over every unique non-epsilon transition out of curr's epsilon
		 * closure.
		 *
		 * This is a set of labels. Since curr->closure is already a closure
		 * (computed on insertion to ml), these labels directly reach the next
		 * states in the NFA.
		 */
		/* TODO: document that nes contains only entries with labels set */
		if (!listnonepsilonstates(&nes, curr->closure)) {
			goto error;
		}

		for (s = nes; s != NULL; s = s->next) {
			struct fsm_state *new;
			struct fsm_edge *e;
			struct set *reachable;

			assert(s->state != NULL);

			reachable = NULL;

			/*
			 * Find the closure of the set of all NFA states which are reachable
			 * through this label, starting from the set of states forming curr's
			 * closure.
			 */
			if (!allstatesreachableby(curr->closure, s->c, &reachable)) {
				set_free(reachable);
				goto error;
			}

			new = set_closure(&ml, dfa, reachable);
			set_free(reachable);
			if (new == NULL) {
				free_transset(nes);
				goto error;
			}

			e = fsm_addedge_literal(dfa, curr->dfastate, new, s->c);
			if (e == NULL) {
				free_transset(nes);
				goto error;
			}
		}

		free_transset(nes);

#ifdef DEBUG_TODFA
		{
			struct set_iter it;
			struct set *q;

			for (q = set_first(curr->closure, &it); q != NULL; q = set_next(&it)) {
				if (!set_add(&curr->dfastate->nfasl, q->state)) {
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
		/* XXX: possibly have callback in fsm struct, instead. like the colour hooks */
		if (carryopaque != NULL) {
			carryopaque(curr->closure, dfa, curr->dfastate);
		}
	}

	free_mappings(ml);

	/* TODO: can assert a whole bunch of things about the dfa, here */
	assert(fsm_all(dfa, fsm_isdfa));

	return dfa;

error:

	free_mappings(ml);
	fsm_free(dfa);

	return NULL;
}

int
fsm_determinise_opaque(struct fsm *fsm,
	void (*carryopaque)(struct set *, struct fsm *, struct fsm_state *))
{
	struct fsm *dfa;

	dfa = determinise(fsm, carryopaque);
	if (dfa == NULL) {
		return 0;
	}

#ifdef DEBUG_TODFA
	fsm->nfa = fsm_new();
	if (fsm->nfa == NULL) {
		return 0;
	}

	assert(fsm->nfa == NULL);
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

int
fsm_determinise(struct fsm *fsm)
{
	return fsm_determinise_opaque(fsm, NULL);
}

