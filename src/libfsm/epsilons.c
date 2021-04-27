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

#include "internal.h"
#include "capture.h"
#include "endids.h"

#define DUMP_EPSILON_CLOSURES 0
#define DEF_PENDING_CAPTURE_ACTIONS_CEIL 2
#define LOG_RM_EPSILONS_CAPTURES 0
#define DEF_CARRY_ENDIDS_COUNT 2

struct remap_env {
	char tag;
	const struct fsm_alloc *alloc;
	struct state_set **rmap;
	int ok;

	size_t count;
	size_t ceil;
	struct remap_action {
		fsm_state_t state;
		enum capture_action_type type;
		unsigned capture_id;
		fsm_state_t to;
	} *actions;
};

static int
remap_capture_actions(struct fsm *nfa, struct state_set **eclosures);

static int
remap_capture_action_cb(fsm_state_t state,
    enum capture_action_type type, unsigned capture_id, fsm_state_t to,
    void *opaque);

static int
carry_endids(struct fsm *fsm, struct state_set *states,
    fsm_state_t s);

static void
free_closure_sets(struct state_set *closures[FSM_SIGMA_COUNT])
{
	int i;

	for (i = 0; i <= FSM_SIGMA_MAX; i++) {
		if (closures[i] != NULL) {
			state_set_free(closures[i]);
		}
	}
}

int
fsm_remove_epsilons(struct fsm *nfa)
{
	struct state_set **eclosures;
	fsm_state_t s;

	assert(nfa != NULL);

	eclosures = epsilon_closure(nfa);
	if (eclosures == NULL) {
		return 0;
	}

#if DUMP_EPSILON_CLOSURES
	{
		struct state_iter kt;
		fsm_state_t es;
		fprintf(stderr, "# ==== epsilon_closures\n");
		for (s = 0; s < nfa->statecount; s++) {
			if (eclosures[s] == NULL) { continue; }
			fprintf(stderr, " -- %u:", s);
			for (state_set_reset(eclosures[s], &kt); state_set_next(&kt, &es); ) {
				fprintf(stderr, " %u", es);
			}
			fprintf(stderr, "\n");
		}
	}
#endif

	for (s = 0; s < nfa->statecount; s++) {
		struct state_set *sclosures[FSM_SIGMA_COUNT] = { NULL };
		struct state_iter kt;
		fsm_state_t es;
		int i;

		if (nfa->states[s].epsilons == NULL) {
			continue;
		}

		for (state_set_reset(eclosures[s], &kt); state_set_next(&kt, &es); ) {
			/* we already have edges from s */
			if (es == s) {
				continue;
			}

			if (!symbol_closure(nfa, es, eclosures, sclosures)) {
				free_closure_sets(sclosures);
				goto error;
			}
		}

		state_set_free(nfa->states[s].epsilons);
		nfa->states[s].epsilons = NULL;

		/* TODO: bail out early if there are no edges to create? */

		for (i = 0; i <= FSM_SIGMA_MAX; i++) {
			if (sclosures[i] == NULL) {
				continue;
			}

			if (!edge_set_add_state_set(&nfa->states[s].edges, nfa->opt->alloc, i, sclosures[i])) {
				free_closure_sets(sclosures);
				goto error;
			}

			/* XXX: we took a copy, but i would prefer to bulk transplant ownership instead */
			state_set_free(sclosures[i]);
			sclosures[i] = NULL; /* prevent double free if we encounter an error */
		}

		/* all elements in sclosures[] have been freed or moved to their
		 * respective newly-created edge, so there's nothing to free here */

		/*
		 * The current NFA state is an end state if any of its associated
		 * epsilon-clousre states are end states.
		 *
		 * Since we're operating on states in-situ, we only need to find
		 * out if the current state isn't already marked as an end state.
		 *
		 * However in either case we still need to carry opaques, because
		 * the set of opaque values may differ.
		 */
		if (!fsm_isend(nfa, s)) {
			if (!state_set_has(nfa,  eclosures[s], fsm_isend)) {
				continue;
			}

			fsm_setend(nfa, s, 1);
		}

		/*
		 * Carry through end IDs, if present. This isn't anything to do
		 * with the NFA conversion; it's meaningful only to the caller.
		 *
		 * The closure may contain non-end states, but at least one state is
		 * known to have been an end state.
		 */
		if (!carry_endids(nfa, eclosures[s], s)) {
			free_closure_sets(sclosures);
			goto error;
		}
	}

	if (!remap_capture_actions(nfa, eclosures)) {
		goto error;
	}

	closure_free(eclosures, nfa->statecount);

	return 1;

error:

	/* TODO: free stuff */
	closure_free(eclosures, nfa->statecount);

	return 0;
}

static int
remap_capture_actions(struct fsm *nfa, struct state_set **eclosures)
{
	int res = 0;
	fsm_state_t s, i;
	struct state_set **rmap;
	struct state_iter si;
	fsm_state_t si_s;
	struct remap_env env = { 'R', NULL, NULL, 1, 0, 0, NULL };
	env.alloc = nfa->opt->alloc;

	/* build a reverse mapping */
	rmap = f_calloc(nfa->opt->alloc, nfa->statecount, sizeof(rmap[0]));
	if (rmap == NULL) {
		goto cleanup;
	}

	for (s = 0; s < nfa->statecount; s++) {
		if (eclosures[s] == NULL) { continue; }
		for (state_set_reset(eclosures[s], &si); state_set_next(&si, &si_s); ) {
			if (si_s == s) {
				continue; /* ignore identical states */
			}
#if LOG_RM_EPSILONS_CAPTURES
			fprintf(stderr, "remap_capture_actions: %u <- %u\n",
			    s, si_s);
#endif
			if (!state_set_add(&rmap[si_s], nfa->opt->alloc, s)) {
				goto cleanup;
			}
		}
	}
	env.rmap = rmap;

	/* Iterate over the current set of actions with the reverse
	 * mapping (containing only states which will be skipped,
	 * collecting info about every new capture action that will need
	 * to be added.
	 *
	 * It can't be added during the iteration, because that would
	 * modify the hash table as it's being iterated over. */
	fsm_capture_action_iter(nfa, remap_capture_action_cb, &env);

	/* Now that we're done iterating, add those actions. */
	for (i = 0; i < env.count; i++) {
		const struct remap_action *a = &env.actions[i];
		if (!fsm_capture_add_action(nfa, a->state, a->type,
			a->capture_id, a->to)) {
			goto cleanup;
		}
	}

	res = 1;

cleanup:
	if (env.actions != NULL) {
		f_free(nfa->opt->alloc, env.actions);
	}

	if (rmap != NULL) {
		for (i = 0; i < nfa->statecount; i++) {
			state_set_free(rmap[i]);
		}
		f_free(nfa->opt->alloc, rmap);
	}
	return res;

}

static int
add_pending_capture_action(struct remap_env *env,
    fsm_state_t state, enum capture_action_type type,
    unsigned capture_id, fsm_state_t to)
{
	struct remap_action *a;
	if (env->count == env->ceil) {
		struct remap_action *nactions;
		const size_t nceil = (env->ceil == 0
		    ? DEF_PENDING_CAPTURE_ACTIONS_CEIL : 2*env->ceil);
		assert(nceil > 0);
		nactions = f_realloc(env->alloc,
		    env->actions,
		    nceil * sizeof(nactions[0]));
		if (nactions == NULL) {
			return 0;
		}

		env->ceil = nceil;
		env->actions = nactions;
	}

	a = &env->actions[env->count];
#if LOG_RM_EPSILONS_CAPTURES
	fprintf(stderr, "add_pending_capture_action: state %d, type %s, capture_id %u, to %d\n",
	    state, fsm_capture_action_type_name[type], capture_id, to);
#endif

	a->state = state;
	a->type = type;
	a->capture_id = capture_id;
	a->to = to;
	env->count++;
	return 1;
}

static int
remap_capture_action_cb(fsm_state_t state,
    enum capture_action_type type, unsigned capture_id, fsm_state_t to,
    void *opaque)
{
	struct state_iter si;
	fsm_state_t si_s;
	struct remap_env *env = opaque;
	assert(env->tag == 'R');

#if LOG_RM_EPSILONS_CAPTURES
	fprintf(stderr, "remap_capture_action_cb: state %d, type %s, capture_id %u, to %d\n",
	    state, fsm_capture_action_type_name[type], capture_id, to);
#endif

	for (state_set_reset(env->rmap[state], &si); state_set_next(&si, &si_s); ) {
		struct state_iter si_to;
		fsm_state_t si_tos;

#if LOG_RM_EPSILONS_CAPTURES
		fprintf(stderr, " -- rcac: state %d -> %d\n", state, si_s);
#endif

		if (!add_pending_capture_action(env, si_s, type, capture_id, to)) {
			goto fail;
		}

		if (to == CAPTURE_NO_STATE) {
			continue;
		}

		for (state_set_reset(env->rmap[to], &si_to); state_set_next(&si, &si_tos); ) {
#if LOG_RM_EPSILONS_CAPTURES
			fprintf(stderr, " -- rcac:     to %d -> %d\n", to, si_tos);
#endif

			if (!add_pending_capture_action(env, si_tos, type, capture_id, to)) {
				goto fail;
			}

		}
	}

	return 1;

fail:
	env->ok = 0;
	return 0;
}

struct collect_env {
	char tag;
	const struct fsm_alloc *alloc;
	size_t count;
	size_t ceil;
	fsm_end_id_t *ids;
	int ok;
};

static int
collect_cb(fsm_state_t state, fsm_end_id_t id, void *opaque)
{
	struct collect_env *env = opaque;
	assert(env->tag == 'E');

	(void)state;

	if (env->count == env->ceil) {
		const size_t nceil = 2 * env->ceil;
		fsm_end_id_t *nids;
		assert(nceil > env->ceil);
		nids = f_realloc(env->alloc, env->ids,
		    nceil * sizeof(*env->ids));
		if (nids == NULL) {
			env->ok = 0;
			return 0;
		}
		env->ceil = nceil;
		env->ids = nids;
	}

	env->ids[env->count] = id;
	env->count++;

	return 1;
}

/* fsm_remove_epsilons can't use fsm_endid_carry directly, because the src
 * and dst FSMs are the same -- that would lead to adding entries to a
 * hash table, possibly causing it to resize, while iterating over it.
 *
 * Instead, collect entries that need to be added (if not already
 * present), and then add them in a second pass. */
static int
carry_endids(struct fsm *fsm, struct state_set *states,
    fsm_state_t dst_state)
{
	struct state_iter it;
	fsm_state_t s;
	size_t i;

	struct collect_env env;
	env.tag = 'E';		/* for fsm_remove_epsilons */
	env.alloc = fsm->opt->alloc;
	env.count = 0;
	env.ceil = DEF_CARRY_ENDIDS_COUNT;
	env.ids = f_malloc(fsm->opt->alloc,
	    env.ceil * sizeof(*env.ids));
	if (env.ids == NULL) {
		return 0;
	}
	env.ok = 1;

	/* collect from states */
	for (state_set_reset(states, &it); state_set_next(&it, &s); ) {
		if (!fsm_isend(fsm, s)) {
			continue;
		}

		fsm_endid_iter_state(fsm, s, collect_cb, &env);
		if (!env.ok) {
			goto cleanup;
		}
	}

	/* add them */
	for (i = 0; i < env.count; i++) {
		enum fsm_endid_set_res sres;
		sres = fsm_endid_set(fsm, dst_state, env.ids[i]);
		if (sres == FSM_ENDID_SET_ERROR_ALLOC_FAIL) {
			env.ok = 0;
			goto cleanup;
		}
	}

cleanup:
	f_free(fsm->opt->alloc, env.ids);

	return env.ok;
}

