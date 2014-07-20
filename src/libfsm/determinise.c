/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/graph.h>

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
	struct state_set *closure;

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
addtoml(struct fsm *dfa, struct mapping **ml, struct state_set *closure)
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
static struct state_set **
epsilon_closure(const struct fsm_state *state, struct state_set **closure)
{
	struct state_set *s;
	struct state_set *e;

	assert(state != NULL);
	assert(closure != NULL);

	/* Find if the given state is already in the closure */
	for (s = *closure; s != NULL; s = s->next) {
		/* If the given state is already in the closure set, we don't need to add it again; end recursion */
		if (s->state == state) {
			return closure;
		}
	}

	/* Create a new entry */
	s = malloc(sizeof *s);
	if (s == NULL) {
		return NULL;
	}

	s->state = (void *) state;

	s->next  = *closure;
	*closure = s;

	/* Follow each epsilon transition */
	for (e = state->edges[FSM_EDGE_EPSILON].sl; e != NULL; e = e->next) {
		assert(e->state != NULL);

		if (epsilon_closure(e->state, closure) == NULL) {
			return NULL;
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
	struct state_set *ec;
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
set_closure(struct mapping **ml, struct fsm *dfa, struct state_set *set)
{
	struct state_set *ec;
	struct mapping *m;
	struct state_set *s;

	assert(ml != NULL);

	ec = NULL;
	for (s = set; s != NULL; s = s->next) {
		if (epsilon_closure(s->state, &ec) == NULL) {
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
listnonepsilonstates(struct transset **l, struct state_set *set)
{
	struct state_set *s;
	struct state_set *e;

	assert(l != NULL);
	assert(set != NULL);

	*l = NULL;
	for (s = set; s != NULL; s = s->next) {
		int i;

		for (i = 0; i <= UCHAR_MAX; i++) {
			for (e = s->state->edges[i].sl; e != NULL; e = e->next) {
				struct transset *p;

				assert(e->state != NULL);

				/* Skip transitions we've already got */
				if (transin(i, *l)) {
					continue;
				}

				p = malloc(sizeof *p);
				if (p == NULL) {
					free_transset(*l);
					return 0;
				}

				p->c = i;
				p->state = e->state;

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
allstatesreachableby(const struct fsm *fsm, struct state_set *set, char c,
	struct state_set **sl)
{
	struct state_set *s;

	assert(set != NULL);

	for (s = set; s != NULL; s = s->next) {
		struct fsm_edge *to;
		struct state_set *es;

		to = &s->state->edges[(unsigned char) c];

		for (es = to->sl; es != NULL; es = es->next) {
			assert(es->state != NULL);

			if (!set_addstate(sl, es->state)) {
				return 0;
			}
		}
	}

	return 1;
}

static void
carryend(struct state_set *set, struct fsm *fsm, struct fsm_state *state)
{
	struct state_set *s;

	assert(set != NULL); /* TODO: right? */
	assert(fsm != NULL);
	assert(state != NULL);

	for (s = set; s != NULL; s = s->next) {
		if (fsm_isend(fsm, s->state)) {
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
 *
 * TODO: returning an int is a little cumbersome here. Why not return an fsm?
 */
static int
determinise(struct mapping **ml, struct fsm *nfa, struct fsm *dfa,
	void (*carryopaque)(struct state_set *, struct fsm *, struct fsm_state *))
{
	struct mapping *curr;

	/*
	 * The epsilon closure of the NFA's start state is the DFA's start state.
	 * This is not yet "done"; it starts off the loop below.
	 */
	{
		struct fsm_state *dfastart;

		dfastart = state_closure(ml, dfa, fsm_getstart(nfa));
		if (dfastart == NULL) {
			/* TODO: error */
			return 0;
		}

		fsm_setstart(dfa, dfastart);
	}

	/*
	 * While there are still DFA states remaining to be "done", process each.
	 */
	for (curr = *ml; (curr = nextnotdone(*ml)) != NULL; curr->done = 1) {
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
			return 0;
		}

		for (s = nes; s != NULL; s = s->next) {
			struct fsm_state *new;
			struct fsm_edge *e;
			struct state_set *reachable;

			assert(s->state != NULL);

			reachable = NULL;

			/*
			 * Find the closure of the set of all NFA states which are reachable
			 * through this label, starting from the set of states forming curr's
			 * closure.
			 */
			if (!allstatesreachableby(nfa, curr->closure, s->c, &reachable)) {
				set_free(reachable);
				return 0;
			}

			new = set_closure(ml, dfa, reachable);
			set_free(reachable);
			if (new == NULL) {
				free_transset(nes);
				return 0;
			}

			e = fsm_addedge_literal(dfa, curr->dfastate, new, s->c);
			if (e == NULL) {
				free_transset(nes);
				return 0;
			}
		}

		free_transset(nes);

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

	return 1;
}

int
fsm_determinise_opaque(struct fsm *fsm,
	void (*carryopaque)(struct state_set *, struct fsm *, struct fsm_state *))
{
	struct mapping *ml;
	struct fsm *dfa;
	int r;

	assert(fsm != NULL);

	dfa = fsm_new();
	if (dfa == NULL) {
		return 0;
	}

	ml = NULL;
	r = determinise(&ml, fsm, dfa, carryopaque);

	free_mappings(ml);
	if (!r) {
		return 0;
	}

	/* TODO: can assert a whole bunch of things about the dfa, here */
	assert(fsm_all(dfa, fsm_isdfa));

	fsm_move(fsm, dfa);

	return 1;
}

int
fsm_determinise(struct fsm *fsm)
{
	return fsm_determinise_opaque(fsm, NULL);
}

