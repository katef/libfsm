/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"
#include "trans.h"

/*
 * A set of states in an NFA.
 *
 * These may optionally have labels naming a transition which was followed to
 * reach the state. This is used for finding which states are reachable by a
 * given label, given an set of several states.
 * TODO: optionally... optional under what condition? maybe split into two types?
 */
struct set {
	const struct fsm_state *state;
	struct trans_list *trans;
	struct set *next;
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
	int done:1;

	struct mapping *next;
};

void free_set(struct set *set) {
	struct set *p;
	struct set *next;

	for (p = set; p; p = next) {
		next = p->next;
		free(p);
	}
}

void free_mappings(struct mapping *m) {
	struct mapping *p;
	struct mapping *next;

	for (p = m; p; p = next) {
		next = p->next;
		free(p);
	}
}

/* Find if a transition is in a set */
static int transin(struct trans_list *trans, const struct set *set) {
	const struct set *p;

	assert(trans != NULL);
	assert(trans->type != FSM_EDGE_EPSILON);

	for (p = set; p; p = p->next) {
		assert(p->trans != NULL);	/* TODO: not sure */
		assert(p->trans->type != FSM_EDGE_EPSILON);	/* TODO: not sure */

		/* TODO: deep comparison? if (trans_equal(p->trans, trans)) { */
		if (p->trans == trans) {
			return 1;
		}
	}

	return 0;
}

/* Find if a state is in a set */
static int statein(const struct fsm_state *state, const struct set *set) {
	const struct set *p;

	for (p = set; p; p = p->next) {
		if (p->state == state) {
			return 1;
		}
	}

	return 0;
}

/* Find a is a subset of b */
static int subsetof(const struct set *a, const struct set *b) {
	const struct set *p;

	for (p = a; p; p = p->next) {
		if (!statein(p->state, b)) {
			return 0;
		}
	}

	return 1;
}

/*
 * Compare two sets of states for equality, ignoring their edge transitions.
 */
static int ecequal(const struct set *a, const struct set *b) {
	return subsetof(a, b) && subsetof(b, a);
}

/*
 * Find the DFA state associated with a given epsilon closure of NFA states.
 * A new DFA state is created if none exists.
 *
 * The association of DFA states to epsilon closures in the NFA is stored in
 * the mapping list ml for future reference.
 */
static struct mapping *addtoml(struct fsm *dfa, struct mapping **ml, struct set *closure) {
	struct mapping *p;

	/* use existing mapping if present */
	for (p = *ml; p; p = p->next) {
		if (ecequal(p->closure, closure)) {
			free_set(closure);
			return p;
		}
	}

	/* else add new DFA state */
	p = malloc(sizeof *p);
	if (p == NULL) {
		return NULL;
	}

	p->closure = closure;
	p->dfastate = fsm_addstate(dfa, 0);
	if (p->dfastate == NULL) {
		free(p);
		return NULL;
	}

	p->done = 0;
	p->next = *ml;
	*ml = p;

	return p;
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
static struct set **epsilon_closure(const struct fsm_state *state, struct set **closure) {
	struct fsm_edge *p;
	struct set *s;

	assert(state != NULL);
	assert(closure != NULL);

	/* Find if the given state is already in the closure */
	for (s = *closure; s; s = s->next) {
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

	s->state = state;
	s->trans = NULL;
	/* TODO: document that this is an entirely unrelated set to the one where ->trans is non-NULL */

	s->next  = *closure;
	*closure = s;

	/* Follow each epsilon transition */
	for (p = state->edges; p; p = p->next) {
		assert(p->trans != NULL);

		if (p->trans->type == FSM_EDGE_EPSILON) {
			if (epsilon_closure(p->state, closure) == NULL) {
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
static struct fsm_state *state_closure(struct mapping **ml, struct fsm *dfa, const struct fsm_state *nfastate) {
	struct set *ec;
	struct mapping *m;

	assert(ml != NULL);
	assert(dfa != NULL);
	assert(nfastate != NULL);

	ec = NULL;
	if (epsilon_closure(nfastate, &ec) == NULL) {
		free_set(ec);
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
static struct fsm_state *set_closure(struct mapping **ml, struct fsm *dfa, struct set *set) {
	struct set *ec;
	struct mapping *m;
	struct set *p;

	assert(ml != NULL);

	ec = NULL;
	for (p = set; p; p = p->next) {
		assert(p->trans == NULL);

		if (epsilon_closure(p->state, &ec) == NULL) {
			free_set(ec);
			return NULL;
		}
	}

	m = addtoml(dfa, ml, ec);
	/* TODO: test ec */

	return m->dfastate;
}

/*
 * Return true if any of the states in the set are end states.
 */
static int containsendstate(struct fsm *fsm, struct set *set) {
	struct set *s;

	assert(fsm != NULL);

	for (s = set; s; s = s->next) {
		if (fsm_isend(fsm, s->state)) {
			return 1;
		}
	}

	return 0;
}

/*
 * Return an arbitary mapping which isn't marked "done" yet.
 */
static struct mapping *nextnotdone(struct mapping *ml) {
	struct mapping *p;

	for (p = ml; p; p = p->next) {
		if (!p->done) {
			return p;
		}
	}

	return NULL;
}

/*
 * List all states within a set which are reachable via non-epsilon
 * transitions (that is, have a label associated with them).
 *
 * Returns l on success, NULL on error.
 * TODO: maybe simpler to just return the set, rather than take a double pointer
 */
static struct set **listnonepsilonstates(struct set **l, struct set *set) {
	struct set *s;
	struct fsm_edge *e;

	assert(l != NULL);
	assert(set != NULL);

	*l = NULL;
	for (s = set; s; s = s->next) {
		for (e = s->state->edges; e; e = e->next) {
			struct set *p;

			assert(e->trans != NULL);

			/* Skip epsilon edges */
			if (e->trans->type == FSM_EDGE_EPSILON) {
				continue;
			}

			/* Skip transitions we've already got */
			if (transin(e->trans, *l)) {
				continue;
			}

			p = malloc(sizeof *p);
			if (p == NULL) {
				free_set(*l);
				return NULL;
			}

			p->state = e->state;
			p->trans = e->trans;

			p->next = *l;
			*l = p;
		}
	}

	return l;
}

/*
 * Return a list of all states reachable from set via the given transition.
 */
static struct set *allstatesreachableby(struct set *set, struct trans_list *trans) {
	struct set *l;
	struct set *s;
	struct fsm_edge *e;

	assert(set != NULL);
	assert(trans != NULL);
	assert(trans->type != FSM_EDGE_EPSILON);

	l = NULL;
	for (s = set; s; s = s->next) {
		for (e = s->state->edges; e; e = e->next) {
			struct set *p;

			assert(e->trans != NULL);

			/* Skip epsilon edges */
			if (e->trans->type == FSM_EDGE_EPSILON) {
				continue;
			}

			/* Skip states which we've already got */
			if (statein(e->state, l)) {
				continue;
			}

			/* Skip labels which aren't the one we're looking for */
			if (!trans_equal(e->trans, trans)) {
				continue;
			}

			p = malloc(sizeof *p);
			if (p == NULL) {
				free_set(l);
				return NULL;
			}

			/*
			 * There is no need to store the label here, since our caller is
			 * only interested in states.
			 */
			p->trans = NULL;
			p->state = e->state;

			p->next = l;
			l = p;
		}
	}

	return l;
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
 * As all DFA are NFA; for a DFA this has no semantic effect other than
 * renumbering states as a side-effect of constructing the new FSM).
 */
static int nfatodfa(struct mapping **ml, struct fsm *nfa, struct fsm *dfa) {
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
		struct set *s;
		struct set *nes;

		/*
		 * Loop over every unique non-epsilon transition out of curr's epsilon
		 * closure.
		 *
		 * This is a set of labels. Since curr->closure is already a closure
		 * (computed on insertion to ml), these labels directly reach the next
		 * states in the NFA.
		 */
		/* TODO: document that nes contains only entries with ->trans set */
		if (listnonepsilonstates(&nes, curr->closure) == NULL) {
			return 0;
		}

		for (s = nes; s; s = s->next) {
			struct fsm_state *new;
			struct set *reachable;
			struct fsm_edge edge;

			assert(s->trans != NULL);
			assert(s->trans->type != FSM_EDGE_EPSILON);

			/*
			 * Find the closure of the set of all NFA states which are reachable
			 * through this label, starting from the set of states forming curr's
			 * closure.
			 */
			reachable = allstatesreachableby(curr->closure, s->trans);

			new = set_closure(ml, dfa, reachable);
			free_set(reachable);
			if (new == NULL) {
				free_set(nes);
				return 0;
			}

			edge.trans = s->trans;	/* XXX hacky */
			if (fsm_addedge_copy(dfa, curr->dfastate, new, &edge) == NULL) {
				free_set(nes);
				return 0;
			}
		}

		free_set(nes);

		/*
		 * The current DFA state is an end state if any of its associated NFA
		 * states are end states.
		 */
		if (containsendstate(nfa, curr->closure)) {
			fsm_setend(dfa, curr->dfastate, 1);
		}
	}

	/* TODO: remove epsilon transition now it's not required */

	return 1;
}

struct fsm *
fsm_todfa(struct fsm *fsm)
{
	int i;
	struct mapping *ml;
	struct fsm *dfa;

	assert(fsm != NULL);

	dfa = fsm_new();
	if (dfa == NULL) {
		return NULL;
	}

	ml = NULL;
	i = nfatodfa(&ml, fsm, dfa);
	free_mappings(ml);
	if (!i) {
		return NULL;
	}

	/* TODO: can assert a whole bunch of things about the dfa, here */
	assert(fsm_isdfa(dfa));

	fsm_move(fsm, dfa);

	return fsm;
}

