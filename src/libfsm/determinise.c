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

struct epsilon_state_data {
	struct fsm_state *st;

	/* these aren't needed after we find the scc */
	unsigned int scc_label;

	union {
		struct {
			unsigned int index;
			unsigned int lowest;
			unsigned int onstack:1;
		} scc;

		struct {
			/* fast way to determine if a state has been included in
			 * an epsilon closure.  if this is equal to the scc
			 * label, then the state has been included.  otherwise,
			 * it has not.  avoids a hash table for membership.
			 */
			unsigned int label;
		} ec;
	} tmp;
};

/* state for finding and memoizing epsilon closures */
struct epsilon_table {
	size_t nstates;
	size_t nscc; /* number of strongly connected components */

	unsigned long curr_closure_id;

	const struct fsm_alloc *alloc;
	struct epsilon_state_data *states;
	struct state_set **scc_states;
	struct state_set **scc_eps_closures;
};

static int 
epsilon_table_initialize(struct epsilon_table *tbl, const struct fsm *fsm)
{
	static const struct epsilon_table zero_tbl;
	static const struct epsilon_state_data zero_data;

	struct fsm_state *st;
	unsigned int i, n;

	assert(tbl != NULL);
	assert(fsm != NULL);

	*tbl = zero_tbl;

	tbl->alloc = fsm->opt->alloc;

	n = fsm_countstates(fsm);
	tbl->states = malloc(n * sizeof tbl->states[0]);
	if (tbl->states == NULL) {
		return -1;
	}

	tbl->nstates = n;

	for (i = 0, st = fsm->sl; st != NULL; i++, st = st->next) {
		st->index = i;
		tbl->states[i] = zero_data;
		tbl->states[i].st  = st;
	}

	return 0;
}

static void
epsilon_table_finalize(struct epsilon_table *tbl)
{
	unsigned int i;

	if (tbl->states != NULL) {
		free(tbl->states);
		tbl->states = NULL;
	}

	if (tbl->scc_states != NULL) {
		for (i=0; i < tbl->nscc; i++) {
			if (tbl->scc_states[i] != NULL) {
				state_set_free(tbl->scc_states[i]);
			}
		}

		free(tbl->scc_states);
		tbl->scc_states = NULL;
	}

	if (tbl->scc_eps_closures != NULL) {
		for (i=0; i < tbl->nscc; i++) {
			if (tbl->scc_eps_closures[i] != NULL) {
				state_set_free(tbl->scc_eps_closures[i]);
			}
		}

		free(tbl->scc_eps_closures);
		tbl->scc_eps_closures = NULL;
	}

	tbl->nstates = 0;
	tbl->nscc = 0;
}

/* data for Tarjan's strongly connected components alg */
struct scc_data {
	unsigned int *stack;

	unsigned int top; /* holds stack top */
	unsigned int lbl; /* holds current scc label */
	unsigned int idx; /* holds current index */
};

static void
scc_dfs(struct epsilon_table *tbl, struct scc_data *data, unsigned int st)
{
	unsigned int lbl;
	struct state_iter it;
	struct state_set *eps;
	struct fsm_state *s;

	assert(st < tbl->nstates);

	/* invariants:
	 * 1. beginning this search, nodes must have been visited (index == 0)
	 * 2. nodes should not be on the node stack (should follow from 1)
	 */
	assert(tbl->states[st].tmp.scc.index   == 0);
	assert(tbl->states[st].tmp.scc.onstack == 0);
	assert(tbl->states[st].scc_label   == 0);

	data->stack[data->top++] = st; /* data->indexes[st]; */

	tbl->states[st].tmp.scc.index = ++data->idx;
	assert(data->idx > 0 && "index overflow");

	tbl->states[st].tmp.scc.lowest = tbl->states[st].tmp.scc.index;
	tbl->states[st].tmp.scc.onstack = 1;

	eps = tbl->states[st].st->epsilons;
	if (eps == NULL) {
		goto label;
	}

	for (s = state_set_first(eps,&it); s != NULL; s = state_set_next(&it)) {
		unsigned int st1;
		st1 = s->index;

		if (tbl->states[st1].tmp.scc.index == 0) {
			scc_dfs(tbl, data, st1);
			if (tbl->states[st].tmp.scc.lowest > tbl->states[st1].tmp.scc.lowest) {
				tbl->states[st].tmp.scc.lowest = tbl->states[st1].tmp.scc.lowest;
			}
		} else if (tbl->states[st1].tmp.scc.onstack && tbl->states[st].tmp.scc.lowest> tbl->states[st1].tmp.scc.index) {
			tbl->states[st].tmp.scc.lowest = tbl->states[st1].tmp.scc.index;
		}
	}

	assert(tbl->states[st].tmp.scc.lowest <= tbl->states[st].tmp.scc.index);
	if (tbl->states[st].tmp.scc.index != tbl->states[st].tmp.scc.lowest) {
		/* not the root of a strongly connected component */
		return;
	}

label:
	/* the root of a strongly connected component */
	lbl = ++data->lbl;
	assert(lbl > 0);
	for (;;) {
		unsigned int st1;

		assert(data->top > 0);
		st1 = data->stack[--data->top];

		assert(tbl->states[st1].tmp.scc.onstack != 0);
		tbl->states[st1].tmp.scc.onstack = 0;

		assert(tbl->states[st1].scc_label == 0);
		tbl->states[st1].scc_label = lbl;

		if (st == st1) {
			break;
		}
	}
}

static int
find_strongly_connected_components(const struct fsm_alloc *a, struct epsilon_table *tbl)
{
	static const struct scc_data zero;
	struct scc_data data;
	unsigned st;

	data = zero;
	data.stack = malloc(tbl->nstates * sizeof data.stack[0]);
	if (data.stack == NULL) {
		goto error;
	}

	for (st=0; st < tbl->nstates; st++) {
		data.stack[st] = 0;
		tbl->states[st].scc_label = 0;

		tbl->states[st].tmp.scc.index   = 0;
		tbl->states[st].tmp.scc.lowest  = 0;
		tbl->states[st].tmp.scc.onstack = 0;
	}

	for (st=0; st < tbl->nstates; st++) {
		if (tbl->states[st].tmp.scc.index > 0) {
			continue;
		}

		if (tbl->states[st].st->epsilons == NULL) {
			/* no epsilon edges, so skip the DFS */
			tbl->states[st].tmp.scc.index = ++data.idx;
			tbl->states[st].scc_label = ++data.lbl;
			continue;
		}

		scc_dfs(tbl, &data, st);

		/* state should be labeled after scc_dfs */
		assert(tbl->states[st].scc_label > 0);
	}

	/* set the labels on the fsm_state nodes */
	{
		unsigned int st;
		for (st=0; st < tbl->nstates; st++) {
			assert(tbl->states[st].st != NULL);
			tbl->states[st].st->eps_scc = tbl->states[st].scc_label;
		}
	}

	/* Build a list of states associated with each SCC so we can generate
	 * the epsilon closure for each SCC in reverse-topological order.
	 */
	{
		unsigned int i,maxlbl;

		tbl->nscc = maxlbl = data.lbl;
		tbl->scc_states = malloc(maxlbl * sizeof tbl->scc_states[0]);
		if (tbl->scc_states == NULL) {
			goto error;
		}

		for (i=0; i < maxlbl; i++) {
			struct state_set *ss;
			ss = state_set_create(a);
			
			if (ss == NULL) {
				goto error;
			}
			tbl->scc_states[i] = ss;
		}

		/* build sets of states */
		for (i=0; i < tbl->nstates; i++) {
			unsigned int lbl = tbl->states[i].scc_label;
			struct fsm_state *s = tbl->states[i].st;

			assert(lbl > 0 && lbl <= maxlbl);
			if (!state_set_add(tbl->scc_states[lbl-1], s)) {
				goto error;
			}
		}
	}

	free(data.stack);
	return 0;

error:
	free(data.stack);

	if (tbl->scc_states != NULL) {
		unsigned int i;
		for (i = 0; i < tbl->nscc; i++) {
			if (tbl->scc_states[i] != NULL) {
				state_set_free(tbl->scc_states[i]);
			}
		}

		free(tbl->scc_states);
		tbl->scc_states = NULL;
		tbl->nscc = 0;
	}

	return -1;
}

static int
build_epsilon_closures(struct fsm *nfa, struct epsilon_table *tbl)
{
	const static struct state_array arr_zero;

	struct state_array arr = arr_zero;
	struct state_set *closure;
	unsigned int i,lbl,nlbl;

	if (find_strongly_connected_components(nfa->opt->alloc,tbl) < 0) {
		return -1;
	}

	for (i=0; i < tbl->nstates; i++) {
		tbl->states[i].tmp.ec.label = 0;
	}

	tbl->scc_eps_closures = calloc(tbl->nscc, sizeof tbl->scc_eps_closures[0]);
	if (tbl->scc_eps_closures == NULL) {
		return -1;
	}

	/* iterate over scc's, building up epsilon closures */

	nlbl = tbl->nscc;
	for (lbl = 0; lbl < nlbl; lbl++) {
		struct state_set *lbl_states;
		struct fsm_state *s;
		struct state_iter it;

		assert(tbl->scc_eps_closures[lbl] == NULL);

		lbl_states = tbl->scc_states[lbl];

		/* add all states in the component to the epsilon closure set */
		for (s = state_set_first(lbl_states, &it); s != NULL; s = state_set_next(&it)) {
			if (!state_array_add(&arr, s)) {
				goto error;
			}

			tbl->states[s->index].tmp.ec.label = lbl+1; /* +1 is intentional!  XXX - document why */
		}

		/* iterate over states reachable from epsilon edges, adding
		 * their closures
		 */

		for (s = state_set_first(lbl_states, &it); s != NULL; s = state_set_next(&it)) {
			struct fsm_state *dst;
			struct state_set *eps;
			struct state_iter jt;

			assert(s->index < tbl->nstates);

			eps = tbl->states[s->index].st->epsilons;
			if (eps == NULL) {
				continue;
			}

			for (dst = state_set_first(eps,&jt); dst != NULL; dst = state_set_next(&jt)) {
				unsigned int dind, dlbl;
				struct state_set *dst_eps;
				struct fsm_state *est;
				struct state_iter kt;

				dind = dst->index;
				assert(dst->index < tbl->nstates);

				dlbl = tbl->states[dst->index].scc_label;
				assert(dlbl > 0 && dlbl <= tbl->nscc);
				assert(dlbl <= lbl+1);

				if (dlbl == lbl+1) {
					/* ignore states in the same scc */
					continue;
				}

				dst_eps = tbl->scc_eps_closures[dlbl-1];
				assert(dst_eps != NULL);

				for (est = state_set_first(dst_eps,&kt); est != NULL; est = state_set_next(&kt)) {
					assert(est->index < tbl->nstates);

					if (tbl->states[est->index].tmp.ec.label == lbl+1) { /* +1 is intentional!  XXX - document why */
						/* already visited */
						continue;
					}

					if (!state_array_add(&arr,est)) {
						goto error;
					}

					/* mark visited */
					tbl->states[est->index].tmp.ec.label = lbl+1;
				}
			}
		}

		/* epsilon closure is complete.  convert into a
		 * state_set and stash it in scc_eps_closure array
		 * for the current label
		 */

		assert(arr.len > 0); /* labels should have at least one state, so the epsilon closure should never be empty! */

		closure = state_set_create_from_array(nfa->opt->alloc, arr.states, arr.len);
		if (closure == NULL) {
			goto error;
		}

		tbl->scc_eps_closures[lbl] = closure;

		/* reset for next closure */
		arr = arr_zero;
	}

	return 0;

error:
	if (arr.len > 0) {
		free(arr.states);
	}

	return -1;
}

int fsm_label_epsilon_scc(struct fsm *fsm)
{
	const static struct epsilon_table epstbl_init;
	struct epsilon_table epstbl = epstbl_init;
	int ret = -1;

	if (epsilon_table_initialize(&epstbl,fsm) < 0) {
		goto finish;
	}

	ret = build_epsilon_closures(fsm, &epstbl);

finish:
	epsilon_table_finalize(&epstbl);
	return ret;
}

/*
 * Return the DFA state associated with the closure of a given NFA state.
 * Create the DFA state if neccessary.
 */
static struct fsm_state *
state_closure(struct epsilon_table *tbl, struct mapping_set *mappings, struct fsm *dfa, const struct fsm_state *nfastate,
	int includeself)
{
	struct mapping *m;
	struct state_set *ec;
	unsigned int scclbl;

	(void)epsilon_closure;  /* eliminates gcc warnings */

	assert(dfa != NULL);
	assert(nfastate != NULL);
	assert(mappings != NULL);

	assert(nfastate->index < tbl->nstates);
	
	scclbl = tbl->states[nfastate->index].scc_label;
	assert(scclbl > 0 && scclbl <= tbl->nscc);
	assert(tbl->scc_eps_closures[scclbl-1] != NULL);

	ec = state_set_copy(tbl->scc_eps_closures[scclbl-1]);
	if (ec == NULL) {
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

static int cmp_single_state(const void *a, const void *b)
{
	const struct fsm_state *ea = a, *eb = b;
	return (ea > eb) - (ea < eb);
}

static unsigned long hash_single_state(const void *a)
{
	const struct fsm_state *st = a;
	return hashrec(st, sizeof *st);
}

/*
 * Return the DFA state associated with the closure of a set of given NFA
 * states. Create the DFA state if neccessary.
 */
static struct fsm_state *
set_closure(struct epsilon_table *tbl, struct mapping_set *mappings, struct fsm *dfa, struct state_set *set)
{
	static const struct state_array zero_arr;

	struct state_iter it;
	struct state_set *ec;
	struct mapping *m;
	struct fsm_state *s;
	struct state_array arr;
	unsigned long curr_closure_id;

	assert(set != NULL);
	assert(mappings != NULL);

	/* possible fast path if set has only one state */
	if (state_set_count(set) == 1) {
		return state_closure(tbl, mappings, dfa, state_set_only(set), 1);
	}

	/* new curr_closure_id */
	curr_closure_id = ++tbl->curr_closure_id;

	arr = zero_arr;

	/*
	memb = hashset_create(tbl->alloc, hash_single_state, cmp_single_state);
	if (memb == NULL) {
		goto error;
	}
	*/

	for (s = state_set_first(set, &it); s != NULL; s = state_set_next(&it)) {
		unsigned int scclbl;
		struct state_set *lc;
		struct fsm_state *dst;
		struct state_iter jt;

		assert(s->index < tbl->nstates);

		scclbl = tbl->states[s->index].scc_label;

		assert(scclbl > 0 && scclbl <= tbl->nscc);

		lc = tbl->scc_eps_closures[scclbl-1];
		for (dst = state_set_first(lc,&jt); dst != NULL; dst = state_set_next(&jt)) {
			if (dst->tmp.eps_closure_id == curr_closure_id) {
				continue;
			}

			dst->tmp.eps_closure_id = curr_closure_id;

			if (!state_array_add(&arr,dst)) {
				goto error;
			}
		}
	}

	assert(arr.len > 0);

	ec = state_set_create_from_array(tbl->alloc, arr.states, arr.len);
	if (ec == NULL) {
		goto error;
	}

	arr.states = NULL;
	arr.len = 0;

	m = addtomappings(mappings, dfa, ec);
	if (m == NULL) {
		state_set_free(ec);
		goto error;
	}

	return m->dfastate;

error:
	if (arr.len > 0) {
		free(arr.states);
	}

	return NULL;
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
buildtransitions(const struct fsm *fsm, struct fsm *dfa, struct epsilon_table *tbl, struct mapping_set *mappings, struct mapping *curr)
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

		new = set_closure(tbl, mappings, dfa, reachable);
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

	struct epsilon_table tbl;

	struct mapping_iter_save sv;

	assert(nfa != NULL);
	assert(nfa->opt != NULL);
	assert(dcache != NULL);

	dfa = fsm_new(nfa->opt);
	if (dfa == NULL) {
		return NULL;
	}

	fsm_clear_tmp(nfa);

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

	if (epsilon_table_initialize(&tbl, nfa) < 0) {
		mapping_set_free(mappings);
		fsm_free(dfa);
		return NULL;
	}

	if (build_epsilon_closures(nfa, &tbl) < 0) {
		epsilon_table_finalize(&tbl);
		mapping_set_free(mappings);
		fsm_free(dfa);
		return NULL;
	}

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

		dfastart = state_closure(&tbl,mappings, dfa, nfastart, includeself);
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
		if (!buildtransitions(nfa, dfa, &tbl, mappings, curr)) {
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

	fsm_clear_tmp(nfa);

	/* TODO: can assert a whole bunch of things about the dfa, here */
	assert(fsm_all(dfa, fsm_isdfa));

	return dfa;

error:

	epsilon_table_finalize(&tbl);
	clear_mappings(dfa, mappings);
	fsm_free(dfa);
	fsm_clear_tmp(nfa);

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

