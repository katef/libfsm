/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>
#include <adt/u64bitset.h>

#include "internal.h"
#include "capture.h"
#include "endids.h"
#include "eager_output.h"

#define DUMP_EPSILON_CLOSURES 0
#define DEF_PENDING_CAPTURE_ACTIONS_CEIL 2
#define LOG_RM_EPSILONS_CAPTURES 0
#define DEF_CARRY_ENDIDS_COUNT 2

#define LOG_LEVEL 0

#if LOG_LEVEL > 0
static bool log_it;
#define LOG(LVL, ...)					\
	do {						\
		if (log_it && LVL <= LOG_LEVEL) {	\
			fprintf(stderr, __VA_ARGS__);	\
		}					\
	} while (0)
#else
#define LOG(_LVL, ...)
#endif

struct remap_env {
#ifndef NDEBUG
	char tag;
#endif
	bool ok;
	const struct fsm_alloc *alloc;
	struct state_set **rmap;

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
mark_states_reachable_by_label(const struct fsm *nfa, uint64_t *reachable_by_label);

struct eager_output_buf {
#define DEF_EAGER_OUTPUT_BUF_CEIL 8
	bool ok;
	const struct fsm_alloc *alloc;
	size_t ceil;
	size_t used;
	fsm_output_id_t *ids;
};

static bool
append_eager_output_id(struct eager_output_buf *buf, fsm_output_id_t id)
{
	if (buf->used == buf->ceil) {
		const size_t nceil = buf->ceil == 0 ? DEF_EAGER_OUTPUT_BUF_CEIL : 2*buf->ceil;
		fsm_output_id_t *nids = f_realloc(buf->alloc, buf->ids, nceil * sizeof(nids[0]));
		if (nids == NULL) {
			buf->ok = false;
			return false;
		}
		buf->ids = nids;
		buf->ceil = nceil;
	}

	for (size_t i = 0; i < buf->used; i++) {
		/* avoid duplicates */
		if (buf->ids[i] == id) { return true; }
	}

	buf->ids[buf->used++] = id;
	return true;
}

static int
collect_eager_output_ids_cb(fsm_state_t state, fsm_output_id_t id, void *opaque)
{
	(void)state;
	struct eager_output_buf *buf = opaque;
	return append_eager_output_id(buf, id) ? 1 : 0;
}

int
fsm_remove_epsilons(struct fsm *nfa)
{
	const size_t state_count = fsm_countstates(nfa);
	int res = 0;
	struct state_set **eclosures = NULL;
	fsm_state_t s;
	struct eager_output_buf eager_output_buf = {
		.ok = true,
		.alloc = nfa->alloc,
	};
	uint64_t *reachable_by_label = NULL;

	LOG(2, "%s: starting\n", __func__);

	INIT_TIMERS();

#if LOG_LEVEL > 0
	log_it = getenv("LOG") != NULL;
#endif

	assert(nfa != NULL);

	TIME(&pre);
	eclosures = epsilon_closure(nfa);
	TIME(&post);
	DIFF_MSEC("epsilon_closure", pre, post, NULL);

	if (eclosures == NULL) {
		goto cleanup;
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

	const size_t state_words = u64bitset_words(state_count);
	reachable_by_label = f_calloc(nfa->alloc, state_words, sizeof(reachable_by_label[0]));
	if (reachable_by_label == NULL) { goto cleanup; }

	mark_states_reachable_by_label(nfa, reachable_by_label);

	fsm_state_t start;
	if (!fsm_getstart(nfa, &start)) {
		goto cleanup;	/* no start state */
	}

	for (s = 0; s < state_count; s++) {
		struct state_iter si;
		fsm_state_t es_id;

		struct edge_group_iter egi;
		struct edge_group_iter_info info;

		/* If the state isn't reachable by a label and isn't the start state,
		 * skip processing -- it will soon become garbage. */
		if (!u64bitset_get(reachable_by_label, s) && s != start) {
			continue;
		}

		/* Process the epsilon closure. */
		state_set_reset(eclosures[s], &si);
		while (state_set_next(&si, &es_id)) {
			struct fsm_state *es = &nfa->states[es_id];

			/* The current NFA state is an end state if any
			 * of its associated epsilon-clousure states are
			 * end states.
			 *
			 * Similarly, any end state metadata on states
			 * in its epsilon-closure is copied to it.
			 *
			 * Capture actions are copied in a later pass. */
			if (fsm_isend(nfa, es_id)) {
#if LOG_COPYING
				fprintf(stderr, "remove_epsilons: setting end on %d (due to %d)\n", s, es_id);
#endif
				fsm_setend(nfa, s, 1);

				/*
				 * Carry through end IDs, if present. This isn't anything to do
				 * with the NFA conversion; it's meaningful only to the caller.
				 */
				if (!carry_endids(nfa, eclosures[s], s)) {
					goto cleanup;
				}
			}

			/* Collect every eager output ID from any state
			 * in the current state's epsilon closure to the
			 * current state. These will be added at the end. */
			{
				if (fsm_eager_output_has_any(nfa, es_id, NULL)) {
					fsm_eager_output_iter_state(nfa, es_id, collect_eager_output_ids_cb, &eager_output_buf);
					if (!eager_output_buf.ok) { goto cleanup; }
				}
			}

			/* For every state in this state's transitive
			 * epsilon closure, add all of their sets of
			 * labeled edges. */
			edge_set_group_iter_reset(es->edges, EDGE_GROUP_ITER_ALL, &egi);
			while (edge_set_group_iter_next(&egi, &info)) {
#if LOG_COPYING
				fprintf(stderr, "%s: bulk-copying edges leading to state %d onto state %d (from state %d)\n",
				    __func__, info.to, s, es_id);
#endif
				if (!edge_set_add_bulk(&nfa->states[s].edges, nfa->alloc,
					info.symbols, info.to)) {
					goto cleanup;
				}
			}
		}

		for (size_t i = 0; i < eager_output_buf.used; i++) {
			if (!fsm_seteageroutput(nfa, s, eager_output_buf.ids[i])) {
				goto cleanup;
			}
		}
		eager_output_buf.used = 0; /* clear */
	}

	/* Remove the epsilon-edge state sets from everything.
	 * This can make states unreachable. */
	for (s = 0; s < state_count; s++) {
		struct fsm_state *state = &nfa->states[s];
		state_set_free(state->epsilons);
		state->epsilons = NULL;
	}

#if LOG_RESULT
	fprintf(stderr, "=== %s: about to update capture actions\n", __func__);
	fsm_dump(stderr, nfa);
#endif

	if (!remap_capture_actions(nfa, eclosures)) {
		goto cleanup;
	}

#if LOG_RESULT
	fsm_dump(stderr, nfa);
	fsm_capture_dump(stderr, "#### post_remove_epsilons", nfa);
#endif

	res = 1;
cleanup:
	LOG(2, "%s: finishing\n", __func__);
	if (eclosures != NULL) {
		closure_free(nfa, eclosures, state_count);
	}
	f_free(nfa->alloc, reachable_by_label);
	f_free(nfa->alloc, eager_output_buf.ids);

	return res;
}

/* For every state, mark every state reached by a labeled edge as
 * reachable. This doesn't check that the FROM state is reachable from
 * the start state (trim will do that soon enough), it's just used to
 * check which states will become unreachable once epsilon edges are
 * removed. We don't need to add eager endids for them, because they
 * will soon be disconnected from the epsilon-free NFA. */
static void
mark_states_reachable_by_label(const struct fsm *nfa, uint64_t *reachable_by_label)
{
	fsm_state_t start;
	if (!fsm_getstart(nfa, &start)) {
		return;		/* nothing reachable */
	}
	u64bitset_set(reachable_by_label, start);

	const fsm_state_t state_count = fsm_countstates(nfa);

	for (size_t s_i = 0; s_i < state_count; s_i++) {
		struct edge_group_iter egi;
		struct edge_group_iter_info info;

		struct fsm_state *s = &nfa->states[s_i];

		/* Clear the visited flag, it will be used to avoid cycles. */
#if 1
		assert(s->visited == 0); /* stale */
#endif
		s->visited = 0;

		edge_set_group_iter_reset(s->edges, EDGE_GROUP_ITER_ALL, &egi);
		while (edge_set_group_iter_next(&egi, &info)) {
			LOG(1, "%s: reachable: %d\n", __func__, info.to);
			u64bitset_set(reachable_by_label, info.to);
		}
	}
}

static int
remap_capture_actions(struct fsm *nfa, struct state_set **eclosures)
{
	int res = 0;
	fsm_state_t s, i;
	struct state_set **rmap;
	struct state_iter si;
	fsm_state_t si_s;
	struct remap_env env = {
#ifndef NDEBUG
		'R',
#endif
		true, NULL, NULL, 0, 0, NULL
	};
	env.alloc = nfa->alloc;

	/* build a reverse mapping */
	rmap = f_calloc(nfa->alloc, nfa->statecount, sizeof(rmap[0]));
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
			if (!state_set_add(&rmap[si_s], nfa->alloc, s)) {
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
		f_free(nfa->alloc, env.actions);
	}

	if (rmap != NULL) {
		for (i = 0; i < nfa->statecount; i++) {
			state_set_free(rmap[i]);
		}
		f_free(nfa->alloc, rmap);
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
	env->ok = false;
	return 0;
}

struct collect_env {
	char tag;
	bool ok;
	const struct fsm_alloc *alloc;
	size_t count;
	size_t ceil;
	fsm_end_id_t *ids;
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
			env->ok = false;
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
	env.alloc = fsm->alloc;
	env.count = 0;
	env.ceil = DEF_CARRY_ENDIDS_COUNT;
	env.ids = f_malloc(fsm->alloc,
	    env.ceil * sizeof(*env.ids));
	if (env.ids == NULL) {
		return 0;
	}
	env.ok = true;

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
		if (!fsm_endid_set(fsm, dst_state, env.ids[i])) {
			env.ok = false;
			goto cleanup;
		}
	}

cleanup:
	f_free(fsm->alloc, env.ids);

	return env.ok;
}
